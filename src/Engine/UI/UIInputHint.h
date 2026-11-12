#pragma once

#include <string>
#include <vector>

#include "Engine/Input/Input.h"
#include "Engine/Resource/FontResource.h"
#include "Engine/Resource/Pointers/ResourcePtr.h"
#include "Engine/UI/InputGlyphs.h"
#include "Engine/UI/UIElement.h"
#include "Engine/Util/Color.h"

namespace Struktur
{
namespace UI
{
// A button prompt for an ACTION ("UIAccept", "Interact", "UITab"), not a fixed key: shows the art for whichever
// binding the player would use right now - keyboard/mouse or pad, whichever was used last, and within that device
// the binding last used for this action (see Input::GetHintSources / InputGlyphs). Re-resolves on its own when that
// changes, resizing to fit, so place it with an anchor point on the side that should stay put.
//
// Optional label text after the glyphs ("[A] Select"), vertically centred with them. Glyph art is drawn at its
// native pixel size times a whole-number scale, so pixel art stays crisp; bindings with no art draw their name as
// "[Name]" text instead.
class UIInputHint : public UIElement
{
public:
	// `action` is an action spec - "UIAccept", or "UITab:negative" for one part (see InputGlyphs::ParseActionSpec).
	UIInputHint(GameContext& context, const glm::vec2& absolutePosition, const glm::vec2& relativePosition,
	            const std::string& action);

	// Layout-affecting setters re-resolve and resize straight away, so a script can read getSize() right after.
	void SetAction(GameContext& context, const std::string& action);
	void SetLabel(GameContext& context, const std::string& label);
	void SetFont(GameContext& context, Resource::ResourcePtr<Resource::FontResource> font);
	void SetFontSize(GameContext& context, float fontSize);
	void SetGlyphScale(GameContext& context, int scale);
	// glyphGap: between two glyphs; labelGap: between the last glyph and the label.
	void SetSpacing(GameContext& context, float glyphGap, float labelGap);
	void SetTextColor(Util::Color color)
	{
		m_textColor   = color;
		m_visualDirty = true;
	}

	const std::string& GetAction() const
	{
		return m_action;
	}

	void Update(GameContext& context) override;
	void Render(GameContext& context) override;
	uint32_t GetRequiredQuadCount() const override;

private:
	// Re-resolves the glyphs for the current device and resizes the element to fit them plus the label.
	void Refresh(GameContext& context);
	float GlyphWidth(const InputGlyph& glyph) const;
	static std::string FallbackLabel(const InputGlyph& glyph)
	{
		return "[" + glyph.text + "]";
	}
	static uint32_t CountTextQuads(const std::string& text);

	std::string m_action;
	Input::InputComponent m_component = Input::InputComponent::None;
	std::string m_label;
	Resource::ResourcePtr<Resource::FontResource> m_font;
	float m_fontSize        = 18.0f;
	Util::Color m_textColor = Util::ColorWhite;
	int m_glyphScale        = 2;
	float m_glyphGap        = 4.0f;
	float m_labelGap        = 8.0f;

	std::vector<InputGlyph> m_glyphs;
	// Input::GetHintRevision() as of the last Refresh - Update() re-resolves when it moves on.
	uint32_t m_seenRevision = 0;
};
}  // namespace UI
}  // namespace Struktur
