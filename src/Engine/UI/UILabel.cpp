#include "UILabel.h"

#include "Engine/GameContext.h"

Struktur::UI::UILabel::UILabel(GameContext& context, const glm::vec2& absolutePosition, const glm::vec2& relativePosition, const std::string& labelText, float fontSz)
    : UIElement(absolutePosition, relativePosition, {0, 0}, {0, 0}), m_text(labelText), m_textColor(BLACK),
    m_alignment(TextAlignment::LEFT), m_wrapping(TextWrapping::NONE), m_fontSize(fontSz)
{
    m_font = context.GetResourceManager().GetFont("default", 32);
    
    // Auto-size based on text
    //::Vector2 textSize = ::MeasureTextEx(m_font->font, m_text.c_str(), m_fontSize, 1.0f);
    //SetSize({textSize.x + 10, textSize.y + 5}, {0, 0}); // Add some padding
    
    // Labels are typically not focusable
    m_focusable = false;
    m_backgroundColor = BLANK; // Transparent by default
    m_borderWidth = 0.0f;
}

void Struktur::UI::UILabel::SetText(const std::string& newText)
{
    m_text = newText;
    // Recalculate size
    //SetBoundingBoxToText();
}

void Struktur::UI::UILabel::SetBoundingBoxToText()
{
    const float extraX = 10.0f;
    const float extraY = 5.0f;

    if (m_text.empty())
    {
        SetSize({ extraX, extraY }, { 0, 0 });
        return;
    }

    float lineHeight = GetLineHeight();
    float maxWidth = 0.0f;
    float totalHeight = 0.0f;

    // Split text by newlines
    size_t start = 0;
    size_t end = 0;
    int lineCount = 0;

    while (end != std::string::npos)
    {
        end = m_text.find('\n', start);

        // Get the substring for this line
        std::string line = (end == std::string::npos)
            ? m_text.substr(start)
            : m_text.substr(start, end - start);

        // Measure this line
        ::Vector2 lineSize = ::MeasureTextEx(m_font->font, line.c_str(), m_fontSize, 1.0f);

        // Track the widest line
        if (lineSize.x > maxWidth)
        {
            maxWidth = lineSize.x;
        }

        lineCount++;
        start = end + 1;
    }

    // Calculate total height: (lineCount - 1) * lineHeight + fontSize for the last line
    if (lineCount > 0)
    {
        totalHeight = (lineCount - 1) * lineHeight + m_fontSize;
    }

    SetSize({ maxWidth + extraX, totalHeight + extraY }, { 0, 0 });
}

void Struktur::UI::UILabel::Update(GameContext &context)
{
    UpdateChildren(context);
}

void Struktur::UI::UILabel::Render(GameContext& context)
{
    // Draw background if not transparent
    if (m_backgroundColor.a > 0)
    {
        ::DrawRectangleRec(m_bounds, m_backgroundColor);
    }
    
    // Draw border if needed
    if (m_borderWidth > 0)
    {
        ::DrawRectangleLinesEx(m_bounds, m_borderWidth, m_borderColor);
    }
    
    // Calculate line height
    float lineHeight = GetLineHeight();
    
    // Calculate starting position
    ::Vector2 startPos = {m_bounds.x + 5, m_bounds.y + 2.5f};
    
    // Render text with wrapping support
    RenderWrappedText(m_text, startPos, lineHeight);
    
    RenderChildren(context);
}

std::vector<std::string> Struktur::UI::UILabel::WrapText(const std::string &text, float maxWidth)
{
    std::vector<std::string> lines;
    
    if (m_wrapping == TextWrapping::NONE)
    {
        lines.push_back(text);
        return lines;
    }
    
    std::string currentLine;
    std::string word;
    
    for (size_t i = 0; i < text.length(); ++i)
    {
        char c = text[i];
        
        // Handle newlines
        if (c == '\n')
        {
            if (!word.empty())
            {
                currentLine += word;
                word.clear();
            }
            lines.push_back(currentLine);
            currentLine.clear();
            continue;
        }
        
        if (m_wrapping == TextWrapping::WORD_WRAP)
        {
            // Word wrap mode
            if (c == ' ' || c == '\t')
            {
                std::string testLine = currentLine + word + c;
                ::Vector2 size = ::MeasureTextEx(m_font->font, testLine.c_str(), m_fontSize, 1.0f);
                
                if (size.x > maxWidth && !currentLine.empty())
                {
                    lines.push_back(currentLine);
                    currentLine = word + c;
                }
                else
                {
                    currentLine = testLine;
                }
                word.clear();
            }
            else
            {
                word += c;
            }
        }
        else if (m_wrapping == TextWrapping::CHARACTER_WRAP)
        {
            // Character wrap mode
            std::string testLine = currentLine + c;
            ::Vector2 size = ::MeasureTextEx(m_font->font, testLine.c_str(), m_fontSize, 1.0f);
            
            if (size.x > maxWidth && !currentLine.empty())
            {
                lines.push_back(currentLine);
                currentLine = std::string(1, c);
            }
            else
            {
                currentLine += c;
            }
        }
    }
    
    // Add remaining content
    if (!word.empty())
    {
        currentLine += word;
    }
    if (!currentLine.empty())
    {
        lines.push_back(currentLine);
    }
    
    return lines;
}

void Struktur::UI::UILabel::RenderJustifiedLine(const std::string &line, ::Vector2 pos, float targetWidth, bool isLastLine)
{
    // Don't justify last line or lines with only one word
    if (isLastLine || line.find(' ') == std::string::npos)
    {
        ::DrawTextEx(m_font->font, line.c_str(), pos, m_fontSize, 1.0f, m_textColor);
        return;
    }
    
    // Count spaces and words
    std::vector<std::string> words;
    std::string currentWord;
    
    for (char c : line)
    {
        if (c == ' ' || c == '\t')
        {
            if (!currentWord.empty())
            {
                words.push_back(currentWord);
                currentWord.clear();
            }
        }
        else
        {
            currentWord += c;
        }
    }
    if (!currentWord.empty())
    {
        words.push_back(currentWord);
    }
    
    if (words.size() <= 1)
    {
        ::DrawTextEx(m_font->font, line.c_str(), pos, m_fontSize, 1.0f, m_textColor);
        return;
    }
    
    // Calculate total word width
    float totalWordWidth = 0;
    for (const auto& word : words)
    {
        ::Vector2 wordSize = ::MeasureTextEx(m_font->font, word.c_str(), m_fontSize, 1.0f);
        totalWordWidth += wordSize.x;
    }
    
    // Calculate space between words
    float totalSpaceWidth = targetWidth - totalWordWidth;
    float spaceWidth = totalSpaceWidth / (words.size() - 1);
    
    // Draw words with calculated spacing
    float currentX = pos.x;
    for (size_t i = 0; i < words.size(); ++i)
    {
        ::Vector2 wordPos = {currentX, pos.y};
        ::DrawTextEx(m_font->font, words[i].c_str(), wordPos, m_fontSize, 1.0f, m_textColor);
        
        ::Vector2 wordSize = ::MeasureTextEx(m_font->font, words[i].c_str(), m_fontSize, 1.0f);
        currentX += wordSize.x + spaceWidth;
    }
}

void Struktur::UI::UILabel::RenderWrappedText(const std::string &text, ::Vector2 startPos, float lineHeight)
{
    float maxWidth = m_bounds.width - 10; // 5px padding on each side
    std::vector<std::string> lines = WrapText(text, maxWidth);
    
    float currentY = startPos.y;
    
    for (size_t i = 0; i < lines.size(); ++i)
    {
        const std::string& line = lines[i];
        ::Vector2 textSize = ::MeasureTextEx(m_font->font, line.c_str(), m_fontSize, 1.0f);
        ::Vector2 textPos = {startPos.x, currentY};
        
        switch (m_alignment)
        {
            case TextAlignment::CENTER:
                textPos.x = m_bounds.x + (m_bounds.width - textSize.x) / 2.0f;
                ::DrawTextEx(m_font->font, line.c_str(), textPos, m_fontSize, 1.0f, m_textColor);
                break;
                
            case TextAlignment::RIGHT:
                textPos.x = m_bounds.x + m_bounds.width - textSize.x - 5;
                ::DrawTextEx(m_font->font, line.c_str(), textPos, m_fontSize, 1.0f, m_textColor);
                break;
                
            case TextAlignment::JUSTIFY:
            {
                textPos.x = m_bounds.x + 5;
                bool isLastLine = (i == lines.size() - 1);
                RenderJustifiedLine(line, textPos, maxWidth, isLastLine);
                break;
            }
                
            case TextAlignment::LEFT:
            default:
                textPos.x = m_bounds.x + 5;
                ::DrawTextEx(m_font->font, line.c_str(), textPos, m_fontSize, 1.0f, m_textColor);
                break;
        }
        
        currentY += lineHeight;
    }
}

::Rectangle Struktur::UI::UILabel::GetFormattedTextBounds() const
{
    ::Vector2 size = GetFormattedTextSize();
    
    // The bounds start at the label's position
    return {
        m_bounds.x,
        m_bounds.y,
        size.x,
        size.y
    };
}

::Vector2 Struktur::UI::UILabel::GetFormattedTextSize() const
{
    if (m_text.empty())
    {
        return {0, 0};
    }
    
    float maxWidth = m_bounds.width - 10; // 5px padding on each side
    std::vector<std::string> lines = const_cast<UILabel*>(this)->WrapText(m_text, maxWidth);
    
    float totalWidth = 0;
    float totalHeight = 0;
    float lineHeight = GetLineHeight();
    
    for (const auto& line : lines)
    {
        ::Vector2 lineSize = ::MeasureTextEx(m_font->font, line.c_str(), m_fontSize, 1.0f);
        
        // For justified text, use the full available width (except last line)
        if (m_alignment == TextAlignment::JUSTIFY && &line != &lines.back())
        {
            totalWidth = std::max(totalWidth, maxWidth);
        }
        else
        {
            totalWidth = std::max(totalWidth, lineSize.x);
        }
        
        totalHeight += lineHeight;
    }
    
    // Add padding
    totalWidth += 10; // 5px on each side
    totalHeight += 5; // 2.5px top + 2.5px bottom
    
    return {totalWidth, totalHeight};
}

float Struktur::UI::UILabel::GetLineHeight() const
{
    return m_fontSize * 1.5f; // 1.5x line spacing for readability
}
