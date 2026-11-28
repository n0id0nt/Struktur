// Workout file loaders (.zwo, .mrc). Files are untrusted input: size-capped before parsing, parsed with pugixml
// (which never fetches external entities or expands DTDs), every number validated, step counts capped, and any
// problem reported as a clear message instead of a partial workout.
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "Ride/Workout.h"
#include "pugixml.hpp"

namespace Struktur::Ride
{
// --- Workout / WorkoutStep ---------------------------------------------------------------------------------------

std::optional<double> WorkoutStep::TargetFractionAt(double secondsIntoStep) const
{
	if (!HasTarget())
	{
		return std::nullopt;
	}
	if (kind == Kind::STEADY || durationSeconds <= 0.0)
	{
		return startFraction;
	}
	const double progress = std::clamp(secondsIntoStep / durationSeconds, 0.0, 1.0);
	return startFraction + (endFraction - startFraction) * progress;
}

double Workout::TotalDurationSeconds() const
{
	double total = 0.0;
	for (const WorkoutStep& step : steps)
	{
		total += step.durationSeconds;
	}
	return total;
}

std::vector<double> Workout::StepStartTimes() const
{
	std::vector<double> starts;
	starts.reserve(steps.size());
	double time = 0.0;
	for (const WorkoutStep& step : steps)
	{
		starts.push_back(time);
		time += step.durationSeconds;
	}
	return starts;
}

// --- helpers ---------------------------------------------------------------------------------------------------

static std::string ToLower(std::string text)
{
	for (char& character : text)
	{
		character = (char)std::tolower((unsigned char)character);
	}
	return text;
}

// std::string paths are read in the ANSI code page on Windows; ours are UTF-8 (SDL drop events, the command line).
static std::filesystem::path PathFromUtf8(const std::string& text)
{
	return std::filesystem::path(std::u8string(text.begin(), text.end()));
}

static std::string Trim(const std::string& text)
{
	const size_t first = text.find_first_not_of(" \t\r\n");
	if (first == std::string::npos)
	{
		return "";
	}
	const size_t last = text.find_last_not_of(" \t\r\n");
	return text.substr(first, last - first + 1);
}

static std::string Clip(const std::string& text, size_t maxLength)
{
	return text.size() > maxLength ? text.substr(0, maxLength) : text;
}

// Strict number parse: the whole (trimmed) string must be one finite number.
static std::optional<double> ParseNumber(const std::string& text)
{
	const std::string trimmed = Trim(text);
	if (trimmed.empty())
	{
		return std::nullopt;
	}
	char* end          = nullptr;
	const double value = std::strtod(trimmed.c_str(), &end);
	if (end == nullptr || *end != '\0' || !std::isfinite(value))
	{
		return std::nullopt;
	}
	return value;
}

// Attribute lookup that ignores case - real-world files mix "Duration"/"duration".
static pugi::xml_attribute FindAttribute(const pugi::xml_node& node, const char* name)
{
	const std::string wanted = ToLower(name);
	for (pugi::xml_attribute attribute : node.attributes())
	{
		if (ToLower(attribute.name()) == wanted)
		{
			return attribute;
		}
	}
	return pugi::xml_attribute();
}

struct ZwoContext
{
	const WorkoutLimits& limits;
	std::string error;
	std::vector<WorkoutStep> steps;
};

static bool Fail(ZwoContext& context, const std::string& message)
{
	if (context.error.empty())
	{
		context.error = message;
	}
	return false;
}

// Reads a required numeric attribute and checks it against [minimum, maximum].
static bool ReadNumber(ZwoContext& context, const pugi::xml_node& node, const char* name, double minimum,
                       double maximum, double& out)
{
	const pugi::xml_attribute attribute = FindAttribute(node, name);
	if (!attribute)
	{
		return Fail(context, std::string("<") + node.name() + "> is missing " + name);
	}
	const std::optional<double> value = ParseNumber(attribute.value());
	if (!value)
	{
		return Fail(context, std::string("<") + node.name() + "> " + name + " is not a number: \"" +
		                         Clip(attribute.value(), 32) + "\"");
	}
	if (*value < minimum || *value > maximum)
	{
		return Fail(context,
		            std::string("<") + node.name() + "> " + name + " is out of range: " + Clip(attribute.value(), 32));
	}
	out = *value;
	return true;
}

static std::optional<double> ReadOptionalNumber(ZwoContext& context, const pugi::xml_node& node, const char* name,
                                                double minimum, double maximum)
{
	if (!FindAttribute(node, name))
	{
		return std::nullopt;
	}
	double value = 0.0;
	if (!ReadNumber(context, node, name, minimum, maximum, value))
	{
		return std::nullopt;
	}
	return value;
}

static bool ReadDuration(ZwoContext& context, const pugi::xml_node& node, const char* name, double& out)
{
	if (!ReadNumber(context, node, name, 0.0, context.limits.maxStepSeconds, out))
	{
		return false;
	}
	if (out <= 0.0)
	{
		return Fail(context, std::string("<") + node.name() + "> " + name + " must be greater than zero");
	}
	return true;
}

static bool ReadPower(ZwoContext& context, const pugi::xml_node& node, const char* name, double& out)
{
	return ReadNumber(context, node, name, 0.0, context.limits.maxPowerFraction, out);
}

static bool AddStep(ZwoContext& context, WorkoutStep step)
{
	if (context.steps.size() >= context.limits.maxSteps)
	{
		return Fail(context, "workout has more than " + std::to_string(context.limits.maxSteps) + " steps");
	}
	context.steps.push_back(std::move(step));
	return true;
}

static void ReadCues(ZwoContext& context, const pugi::xml_node& node, WorkoutStep& step)
{
	for (pugi::xml_node child : node.children())
	{
		if (ToLower(child.name()) != "textevent" || step.cues.size() >= context.limits.maxCuesPerStep)
		{
			continue;
		}
		const std::optional<double> offset =
		    ReadOptionalNumber(context, child, "timeoffset", 0.0, context.limits.maxStepSeconds);
		const pugi::xml_attribute message = FindAttribute(child, "message");
		if (!offset || !message)
		{
			continue;  // distance-based or malformed cue: skipped rather than failing the whole workout
		}
		TextCue cue;
		cue.offsetSeconds = *offset;
		cue.message       = Clip(message.value(), context.limits.maxTextLength);
		if (const std::optional<double> duration = ReadOptionalNumber(context, child, "duration", 0.0, 3600.0))
		{
			cue.durationSeconds = *duration;
		}
		step.cues.push_back(std::move(cue));
	}
}

static bool ParseZwoElement(ZwoContext& context, const pugi::xml_node& node)
{
	const std::string tag = ToLower(node.name());
	WorkoutStep step;
	step.cadenceRpm = ReadOptionalNumber(context, node, "Cadence", 0.0, 250.0);

	if (tag == "warmup" || tag == "cooldown" || tag == "ramp")
	{
		double low  = 0.0;
		double high = 0.0;
		if (!ReadDuration(context, node, "Duration", step.durationSeconds))
		{
			return false;
		}
		if (FindAttribute(node, "PowerLow") || FindAttribute(node, "PowerHigh"))
		{
			if (!ReadPower(context, node, "PowerLow", low) || !ReadPower(context, node, "PowerHigh", high))
			{
				return false;
			}
		}
		else if (!ReadPower(context, node, "Power", low))
		{
			return false;
		}
		else
		{
			high = low;
		}
		step.kind  = low == high ? WorkoutStep::Kind::STEADY : WorkoutStep::Kind::RAMP;
		step.label = tag == "warmup" ? "Warmup" : (tag == "cooldown" ? "Cooldown" : "Ramp");
		if (tag == "cooldown")
		{
			// Cooldown always ramps DOWN: files disagree on which attribute holds the start, so take the higher one.
			step.startFraction = std::max(low, high);
			step.endFraction   = std::min(low, high);
		}
		else
		{
			step.startFraction = low;
			step.endFraction   = high;
		}
		ReadCues(context, node, step);
		return context.error.empty() && AddStep(context, std::move(step));
	}

	if (tag == "steadystate")
	{
		double power = 0.0;
		if (!ReadDuration(context, node, "Duration", step.durationSeconds))
		{
			return false;
		}
		if (FindAttribute(node, "Power"))
		{
			if (!ReadPower(context, node, "Power", power))
			{
				return false;
			}
		}
		else
		{
			double low  = 0.0;
			double high = 0.0;
			if (!ReadPower(context, node, "PowerLow", low) || !ReadPower(context, node, "PowerHigh", high))
			{
				return false;
			}
			power = 0.5 * (low + high);
		}
		step.kind          = WorkoutStep::Kind::STEADY;
		step.startFraction = power;
		step.endFraction   = power;
		step.label         = "Steady";
		ReadCues(context, node, step);
		return context.error.empty() && AddStep(context, std::move(step));
	}

	if (tag == "intervalst")
	{
		double repeat      = 0.0;
		double onDuration  = 0.0;
		double offDuration = 0.0;
		double onPower     = 0.0;
		double offPower    = 0.0;
		if (!ReadNumber(context, node, "Repeat", 1.0, (double)context.limits.maxRepeat, repeat) ||
		    !ReadDuration(context, node, "OnDuration", onDuration) ||
		    !ReadDuration(context, node, "OffDuration", offDuration) || !ReadPower(context, node, "OnPower", onPower) ||
		    !ReadPower(context, node, "OffPower", offPower))
		{
			return false;
		}
		if (repeat != std::floor(repeat))
		{
			return Fail(context, "<IntervalsT> Repeat must be a whole number");
		}
		const std::optional<double> restingCadence = ReadOptionalNumber(context, node, "CadenceResting", 0.0, 250.0);
		const int count                            = (int)repeat;
		// Cues are written against the whole IntervalsT block; hand each one to the on/off step it falls in.
		WorkoutStep cueSource;
		ReadCues(context, node, cueSource);
		const double cycle = onDuration + offDuration;
		for (int i = 0; i < count; ++i)
		{
			WorkoutStep on;
			on.kind            = WorkoutStep::Kind::STEADY;
			on.durationSeconds = onDuration;
			on.startFraction   = onPower;
			on.endFraction     = onPower;
			on.cadenceRpm      = step.cadenceRpm;
			on.label           = "Interval " + std::to_string(i + 1) + "/" + std::to_string(count);
			WorkoutStep off;
			off.kind            = WorkoutStep::Kind::STEADY;
			off.durationSeconds = offDuration;
			off.startFraction   = offPower;
			off.endFraction     = offPower;
			off.cadenceRpm      = restingCadence;
			off.label           = "Recovery " + std::to_string(i + 1) + "/" + std::to_string(count);
			for (const TextCue& cue : cueSource.cues)
			{
				const double blockStart = i * cycle;
				if (cue.offsetSeconds >= blockStart && cue.offsetSeconds < blockStart + onDuration)
				{
					TextCue moved       = cue;
					moved.offsetSeconds = cue.offsetSeconds - blockStart;
					on.cues.push_back(moved);
				}
				else if (cue.offsetSeconds >= blockStart + onDuration && cue.offsetSeconds < blockStart + cycle)
				{
					TextCue moved       = cue;
					moved.offsetSeconds = cue.offsetSeconds - blockStart - onDuration;
					off.cues.push_back(moved);
				}
			}
			if (!AddStep(context, std::move(on)) || !AddStep(context, std::move(off)))
			{
				return false;
			}
		}
		return context.error.empty();
	}

	if (tag == "freeride" || tag == "maxeffort")
	{
		if (!ReadDuration(context, node, "Duration", step.durationSeconds))
		{
			return false;
		}
		step.kind  = tag == "freeride" ? WorkoutStep::Kind::FREE_RIDE : WorkoutStep::Kind::MAX_EFFORT;
		step.label = tag == "freeride" ? "Free Ride" : "Max Effort";
		ReadCues(context, node, step);
		return context.error.empty() && AddStep(context, std::move(step));
	}

	// Anything else (a newer element we don't know, a stray textevent at workout level) is ignored.
	return true;
}

static WorkoutLoadResult Failure(const std::string& message)
{
	WorkoutLoadResult result;
	result.error = message;
	return result;
}

static WorkoutLoadResult FinishWorkout(Workout workout, const WorkoutLimits& limits)
{
	if (workout.steps.empty())
	{
		return Failure("the workout has no steps");
	}
	if (workout.TotalDurationSeconds() > limits.maxTotalSeconds)
	{
		return Failure("the workout is longer than " + std::to_string((int)(limits.maxTotalSeconds / 3600)) + " hours");
	}
	if (workout.name.empty())
	{
		workout.name = "Workout";
	}
	WorkoutLoadResult result;
	result.workout = std::move(workout);
	return result;
}

// --- ZWO -------------------------------------------------------------------------------------------------------

WorkoutLoadResult ParseZwo(const std::string& text, const WorkoutLimits& limits)
{
	if (text.size() > limits.maxFileBytes)
	{
		return Failure("the file is larger than " + std::to_string(limits.maxFileBytes / 1024) + " KB");
	}
	pugi::xml_document document;
	// parse_default never processes the DOCTYPE or fetches external entities.
	const pugi::xml_parse_result parsed = document.load_buffer(text.data(), text.size(), pugi::parse_default);
	if (!parsed)
	{
		return Failure(std::string("not valid XML: ") + parsed.description() + " (byte " +
		               std::to_string(parsed.offset) + ")");
	}

	pugi::xml_node root;
	for (pugi::xml_node child : document.children())
	{
		if (ToLower(child.name()) == "workout_file")
		{
			root = child;
		}
	}
	if (!root)
	{
		return Failure("not a .zwo workout (no <workout_file> element)");
	}

	Workout workout;
	pugi::xml_node steps;
	for (pugi::xml_node child : root.children())
	{
		const std::string tag = ToLower(child.name());
		if (tag == "name")
		{
			workout.name = Clip(Trim(child.text().get()), limits.maxTextLength);
		}
		else if (tag == "author")
		{
			workout.author = Clip(Trim(child.text().get()), limits.maxTextLength);
		}
		else if (tag == "description")
		{
			workout.description = Clip(Trim(child.text().get()), limits.maxTextLength * 4);
		}
		else if (tag == "sporttype")
		{
			const std::string sport = ToLower(Trim(child.text().get()));
			if (!sport.empty() && sport != "bike")
			{
				return Failure("this is a " + Clip(sport, 16) + " workout, not a bike workout");
			}
		}
		else if (tag == "workout")
		{
			steps = child;
		}
	}
	if (!steps)
	{
		return Failure("the file has no <workout> section");
	}

	ZwoContext context{limits, {}, {}};
	for (pugi::xml_node element : steps.children())
	{
		if (element.type() != pugi::node_element)
		{
			continue;
		}
		if (!ParseZwoElement(context, element))
		{
			return Failure(context.error.empty() ? "invalid workout step" : context.error);
		}
	}
	workout.steps = std::move(context.steps);
	return FinishWorkout(std::move(workout), limits);
}

// --- MRC -------------------------------------------------------------------------------------------------------

WorkoutLoadResult ParseMrc(const std::string& text, const WorkoutLimits& limits)
{
	if (text.size() > limits.maxFileBytes)
	{
		return Failure("the file is larger than " + std::to_string(limits.maxFileBytes / 1024) + " KB");
	}
	std::istringstream lines(text);
	std::string line;
	enum class Section
	{
		NONE,
		HEADER,
		DATA,
		OTHER,
	};
	Section section     = Section::NONE;
	bool sawHeader      = false;
	bool minutesPercent = false;
	Workout workout;
	std::vector<std::pair<double, double>> points;  // minute, percent
	int lineNumber = 0;

	while (std::getline(lines, line))
	{
		++lineNumber;
		const std::string trimmed = Trim(line);
		const std::string lower   = ToLower(trimmed);
		if (trimmed.empty() || trimmed[0] == ';')
		{
			continue;
		}
		if (lower == "[course header]")
		{
			section   = Section::HEADER;
			sawHeader = true;
			continue;
		}
		if (lower == "[course data]")
		{
			section = Section::DATA;
			continue;
		}
		if (lower.rfind("[end", 0) == 0)
		{
			section = Section::NONE;
			continue;
		}
		if (lower[0] == '[')
		{
			section = Section::OTHER;  // e.g. [COURSE TEXT] - not used
			continue;
		}

		if (section == Section::HEADER)
		{
			const size_t equals = trimmed.find('=');
			if (equals != std::string::npos)
			{
				const std::string key   = ToLower(Trim(trimmed.substr(0, equals)));
				const std::string value = Clip(Trim(trimmed.substr(equals + 1)), limits.maxTextLength);
				if (key == "description")
				{
					workout.description = value;
				}
				else if (key == "file name")
				{
					workout.name = value;
				}
			}
			else if (lower.find("minutes") != std::string::npos)
			{
				if (lower.find("watts") != std::string::npos)
				{
					return Failure("absolute-watts (MINUTES WATTS) courses aren't supported - use MINUTES PERCENT");
				}
				minutesPercent = lower.find("percent") != std::string::npos;
			}
		}
		else if (section == Section::DATA)
		{
			std::istringstream fields(trimmed);
			std::string minuteText;
			std::string percentText;
			fields >> minuteText >> percentText;
			const std::optional<double> minute  = ParseNumber(minuteText);
			const std::optional<double> percent = ParseNumber(percentText);
			if (!minute || !percent)
			{
				return Failure("course data line " + std::to_string(lineNumber) + " is not \"minutes percent\"");
			}
			if (*minute < 0.0 || *percent < 0.0 || *percent > limits.maxPowerFraction * 100.0)
			{
				return Failure("course data line " + std::to_string(lineNumber) + " is out of range");
			}
			if (!points.empty() && *minute < points.back().first)
			{
				return Failure("course data line " + std::to_string(lineNumber) + " goes back in time");
			}
			if (points.size() > limits.maxSteps * 2)
			{
				return Failure("the course has too many data points");
			}
			points.emplace_back(*minute, *percent);
		}
	}

	if (!sawHeader)
	{
		return Failure("not an .mrc course (no [COURSE HEADER])");
	}
	if (!minutesPercent)
	{
		return Failure("the course header doesn't declare MINUTES PERCENT columns");
	}
	if (points.size() < 2)
	{
		return Failure("the course needs at least two data points");
	}

	for (size_t i = 0; i + 1 < points.size(); ++i)
	{
		const double duration = (points[i + 1].first - points[i].first) * 60.0;
		if (duration <= 0.0)
		{
			continue;  // a vertical jump between two levels
		}
		if (duration > limits.maxStepSeconds)
		{
			return Failure("a course segment is longer than the step limit");
		}
		if (workout.steps.size() >= limits.maxSteps)
		{
			return Failure("the course has more than " + std::to_string(limits.maxSteps) + " steps");
		}
		WorkoutStep step;
		step.durationSeconds = duration;
		step.startFraction   = points[i].second / 100.0;
		step.endFraction     = points[i + 1].second / 100.0;
		step.kind  = step.startFraction == step.endFraction ? WorkoutStep::Kind::STEADY : WorkoutStep::Kind::RAMP;
		step.label = step.kind == WorkoutStep::Kind::STEADY ? "Steady" : "Ramp";
		workout.steps.push_back(std::move(step));
	}
	return FinishWorkout(std::move(workout), limits);
}

// --- dispatch --------------------------------------------------------------------------------------------------

static std::string Extension(const std::string& fileName)
{
	const size_t dot = fileName.find_last_of('.');
	return dot == std::string::npos ? "" : ToLower(fileName.substr(dot + 1));
}

WorkoutLoadResult ParseWorkout(const std::string& fileName, const std::string& text, const WorkoutLimits& limits)
{
	const std::string extension = Extension(fileName);
	WorkoutLoadResult result;
	if (extension == "zwo")
	{
		result = ParseZwo(text, limits);
	}
	else if (extension == "mrc")
	{
		result = ParseMrc(text, limits);
	}
	else
	{
		return Failure("unsupported file type ." + Clip(extension, 8) + " (use .zwo or .mrc)");
	}
	if (result.workout)
	{
		result.workout->sourcePath = fileName;
		if (result.workout->name == "Workout")
		{
			// Fall back to the file's own name when the file names nothing.
			result.workout->name = Clip(PathFromUtf8(fileName).stem().string(), limits.maxTextLength);
		}
	}
	return result;
}

WorkoutLoadResult LoadWorkoutFile(const std::string& path, const WorkoutLimits& limits)
{
	std::error_code error;
	const std::filesystem::path filePath = PathFromUtf8(path);
	const uintmax_t size                 = std::filesystem::file_size(filePath, error);
	if (error)
	{
		return Failure("can't read the file (" + error.message() + ")");
	}
	if (size > limits.maxFileBytes)
	{
		return Failure("the file is larger than " + std::to_string(limits.maxFileBytes / 1024) + " KB");
	}
	std::ifstream file(filePath, std::ios::binary);
	if (!file)
	{
		return Failure("can't open the file");
	}
	std::string text((size_t)size, '\0');
	file.read(text.data(), (std::streamsize)size);
	return ParseWorkout(path, text, limits);
}
}  // namespace Struktur::Ride
