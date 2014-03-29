#include "textmenu.h"
#include <sstream>

std::string const TextMenu::KEYS = "1234567890abcdefghijklmnopqrstuvwxyz";
TextMenu::TextMenu() : text(nullptr), font(0), entries()
{
}

TextMenu::~TextMenu()
{
  glhckTextFree(text);
}

void TextMenu::addOption(int value, const std::string& label, int order)
{
  entries.insert({order, {value, label}});
}

void TextMenu::clear()
{
  entries.clear();
}

void TextMenu::update()
{
  if(text == nullptr)
  {
    text = glhckTextNew(512, 512);
    int nativeSize;
    font = glhckTextFontNewKakwafont(text, &nativeSize);
  }
  glhckTextClear(text);
  int i = 0;
  for(auto const& item : entries)
  {
    Entry const& entry = item.second;
    std::ostringstream oss;
    oss << KEYS.at(i) << ": " << entry.label;
    glhckTextStash(text, font, 24, 4,
                   28 + i * 24, oss.str().data(), nullptr);
    ++i;
  }
}

bool TextMenu::input(const std::string& c, int* result)
{
  std::string::size_type pos = KEYS.find_first_of(c);
  if(pos == std::string::npos)
    return false;


  int i = 0;
  for(auto const& item : entries)
  {
    if(i == pos)
    {
      *result = item.second.value;
      return true;
    }
    ++i;
  }
  return false;
}

bool TextMenu::input(int key, int* result)
{
  std::string s = " ";
  s.at(0) = key;
  return input(s, result);
}

void TextMenu::render()
{
  if(text != nullptr)
  {
    glhckTextRender(text);
  }
}
