"""Independent check of a FIT activity written by the ride screens.

Decodes the file with fitdecode (MIT, https://github.com/polyvertex/fitdecode) - a decoder that shares no code with
the game's encoder - and checks the structure an upload site expects: CRCs, file_id type, monotonic record
timestamps, one session whose num_laps matches the lap messages, and an activity message.

    python -m pip install fitdecode==0.10.0
    python tools/validate_fit.py path/to/ride.fit [more.fit ...]

Exit code 0 when every file passes.
"""
import sys

import fitdecode


def validate(path):
    problems = []
    counts = {}
    timestamps = []
    laps = 0
    session = None
    file_type = None
    with fitdecode.FitReader(path, check_crc=fitdecode.CrcCheck.RAISE) as reader:
        for frame in reader:
            if frame.frame_type != fitdecode.FIT_FRAME_DATA:
                continue
            counts[frame.name] = counts.get(frame.name, 0) + 1
            if frame.name == "file_id":
                file_type = frame.get_value("type")
            elif frame.name == "record":
                timestamps.append(frame.get_value("timestamp"))
                for field in ("power", "distance", "speed", "altitude"):
                    if not frame.has_field(field):
                        problems.append("record without %s" % field)
                        break
            elif frame.name == "lap":
                laps += 1
            elif frame.name == "session":
                session = frame
    if file_type != "activity":
        problems.append("file_id.type is %r, expected 'activity'" % file_type)
    if not timestamps:
        problems.append("no record messages")
    if any(later < earlier for earlier, later in zip(timestamps, timestamps[1:])):
        problems.append("record timestamps go backwards")
    if counts.get("session") != 1:
        problems.append("expected exactly one session, found %s" % counts.get("session"))
    elif session.get_value("num_laps") != laps:
        problems.append("session.num_laps %s != %s lap messages" % (session.get_value("num_laps"), laps))
    if counts.get("activity") != 1:
        problems.append("expected exactly one activity message")
    summary = ", ".join("%s=%d" % item for item in sorted(counts.items()))
    return problems, summary, session


def main(paths):
    if not paths:
        print(__doc__)
        return 2
    failed = False
    for path in paths:
        try:
            problems, summary, session = validate(path)
        except Exception as error:  # a CRC or framing error from the decoder
            problems, summary, session = ["decoder rejected the file: %s" % error], "", None
        status = "OK" if not problems else "FAIL"
        print("%s  %s  (%s)" % (status, path, summary))
        if session is not None:
            print("    session: sport=%s/%s timer=%ss distance=%sm avg_power=%s avg_hr=%s laps=%s" % (
                session.get_value("sport"), session.get_value("sub_sport"),
                session.get_value("total_timer_time"), session.get_value("total_distance"),
                session.get_value("avg_power"), session.get_value("avg_heart_rate"), session.get_value("num_laps")))
        for problem in problems:
            print("    - " + problem)
        failed = failed or bool(problems)
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
