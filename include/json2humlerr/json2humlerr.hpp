#pragma once

#include <fmt/color.h>
#include <json.hpp>

#include <config.hpp>

#include <algorithm>
#include <iostream>
#include <iterator>
#include <fstream>
#include <sstream>
#include <cctype>

void process_value(json::json& msg);

/*
    ┌── Error: 5234 ──────────────────────────────────────────┒
    │ <file.cpp>                                              ┃
┌───┘ ..                                                      ┃
│ 23  auto file = xyz.induce_error(42)                        ┃
└───┐             ╍┳╍              ┳╍                         ┃
    │              ╏               ╏                          ┃
    │              ╏               ┗━ This should be another  ┃
    │              ╏                  number.                 ┃
    │              ╏                                          ┃
    │              ┗━ This is a rather long explanation       ┃
    │                 due to this identifier "xyz".           ┃
    │ ..                                                      ┃
┌───┘ ..                                                      ┃
│ 24                 .just_right_now() - 13;                  ┃
└───┐                 ┳╍╍╍╍╍╍╍╍╍╍╍╍╍╍╍╍╍╍╍╍                   ┃
    │                 ╏                  ╍┳                   ┃
    │                 ╏                   ┗╍ See ⛐            ┃
    │                 ╏                                       ┃
    │                 ┗╍ These two messages overlap, but      ┃
    │                    this is not a too big deal.          ┃
    │                    Interesting for more overlaps.       ┃
    │ ..                                                      ┃
    │                                                         ┃
    ├─ Continued Messages:                                   ─┨
    │ - ⛐  This message is way too long to fit in just        ┃
    │      one single line inside this box, however, it       ┃
    │      is a very informational error message.             ┃
    ├─                                                       ─┨
    │ ◦ Additional message with some more general info that   ┃
    │   is not related to any kind of position.               ┃
    │ ◦ Another additional message.                           ┃
    ├─                                                       ─┨
    │ 🖝  Read the wiki: https://my_cool_lang.wiki/error/5234  ┃
    ┕━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
*/

namespace detail
{
  void process_value(json::json& msg);
}

void process_value(json::json& msg)
{
  detail::process_value(msg);
}

namespace detail
{

void print_top_box(std::size_t line_width, json::json& msg);
void print_file_sep_begin(std::size_t line_width);
void print_file_sep_inner(std::size_t line_width, std::ifstream& file, json::json& msg, std::vector<std::vector<std::string>>& too_long_submessages);
void print_file_sep_end(std::size_t line_width);
void print_box_sep(std::size_t line_width, std::string with_text);
void print_continued_messages(std::size_t line_width, std::vector<std::vector<std::string>>& too_long_submessages);
void print_wiki_line(std::size_t line_width, json::json& msg);
void print_unpos_msgs(std::size_t line_width, json::json& msgs);
void print_bot_box(std::size_t line_width);
std::uint_fast32_t min_num_len(json::json& submsgs);

void process_value(json::json& msg)
{
  const std::size_t line_num_width = min_num_len(msg["msgs"]);

  //      ┌── Error: 5234 ──────────────────────────────────────────┒
  //      │ <file.cpp>                                              ┃
  print_top_box(line_num_width, msg);

  std::vector<std::vector<std::string>> too_long_submessages;
  std::ifstream message_file(msg["file"].get<std::string>());
  for(auto& v : msg["msgs"])
  {
    message_file.seekg(0, std::ios::beg);
    //┌───┘ ..                                                      ┃
    print_file_sep_begin(line_num_width);
    //│ 23  auto file = xyz.induce_error(42)                        ┃
    //└───┐             ╍┳╍              ┳╍                         ┃
    print_file_sep_inner(line_num_width, message_file, v, too_long_submessages);
    //    │ ..                                                      ┃
    print_file_sep_end(line_num_width);
  }
  if(!too_long_submessages.empty())
  {
    print_box_sep(line_num_width, "Continued Messages:");
    print_continued_messages(line_num_width, too_long_submessages);
  }
  print_box_sep(line_num_width, "");
  if(msg.contains("unpos_msgs"))
  {
    print_unpos_msgs(line_num_width, msg["unpos_msgs"]);
    print_box_sep(line_num_width, "");
  }
  print_wiki_line(line_num_width, msg);
  print_bot_box(line_num_width);
}

////////////////////////////////////////

std::uint_fast32_t num_len_as_string(std::uint_fast32_t num)
{ return std::uint_fast32_t(std::log10(num)) + 1; }

std::uint_fast32_t min_num_len(json::json& submsgs)
{
  std::uint_fast32_t max_val = 0;
  for(auto& v : submsgs)
    max_val = std::max(max_val, num_len_as_string(v["loc"]["row_beg"]));
  return max_val;
}

void print_file_sep_begin(std::size_t line_num_width)
{
  std::string to_print;

  std::size_t width = 3;
  to_print = to_print + box_bot_right + box_left_right + box_left_right;
  for(std::size_t i = 0; i < line_num_width; ++i)
    to_print += box_left_right;
  width += line_num_width + 2 + std::string(submsg_separator).size();
  to_print = to_print + box_top_left + " " + submsg_separator;

  if(line_width > width + 1)
    for(std::size_t i = 0; i < line_width - width; ++i)
      to_print += " ";

  to_print = to_print + box_topb_botb + "\n";
  fmt::print(to_print);
}


// splits the message according to the distance to the box starting from beg
// if the message is too long and we cannot split meaningfully, the vector will be empty
// a split is "meaningfull" if it happens at a whitespace
std::vector<std::string> split_message(std::size_t beg, std::string message, std::size_t l_width)
{
  if(message.size() + beg < l_width)
    return { message };
  std::istringstream iss(message);
  std::vector<std::string> results((std::istream_iterator<std::string>(iss)),
                                   std::istream_iterator<std::string>());
  if(std::any_of(results.begin(), results.end(), [beg, l_width](auto s) { return s.size() + beg >= l_width; }))
    return {}; // can't split with given constraints
  std::vector<std::string> splitted;
  for(std::size_t w_idx = 0; w_idx < results.size(); )
  {
    std::string buf;
    for(; w_idx < results.size(); )
    {
      const std::size_t additional_size = results[w_idx].size();

      if(buf.size() + beg + additional_size + (!buf.empty() ? 1 : 0) < l_width)
      {
        if(!buf.empty())
          buf.push_back(' ');
        buf = buf + results[w_idx++];
      }
      else
        break;
    }
    splitted.emplace_back(buf);
  }
  return splitted;
}

void print_unpos_msgs(std::size_t line_num_width, json::json& msgs)
{
  std::string to_print;

  std::size_t width = 3 + line_num_width + 2 + 2;
  for(auto& v : msgs)
  {
    to_print = "   ";
    for(std::size_t i = 0; i < line_num_width; ++i)
      to_print += " ";
    to_print = to_print + box_top_bot + "  ";

    to_print += bullet;
    auto the_msg = v["msg"].get<std::string>();
    auto splitted = split_message(width, the_msg, line_width - 1);
    
    assert(!splitted.empty() && "must not be empty");
    for(auto it = splitted.begin(); it != splitted.end(); ++it)
    {
      if(it != splitted.begin())
      {
        to_print = "   ";
        for(std::size_t i = 0; i < line_num_width; ++i)
          to_print += " ";
        to_print = to_print + box_top_bot + "    ";
      }
      to_print += *it;
      for(std::size_t i = 0; i < line_width - width - it->size() - 1; ++i)
        to_print += " ";

      to_print += box_topb_botb;
      to_print += "\n";
      fmt::print(to_print);
    }
  }
}

void print_file_sep_inner(std::size_t line_num_width, std::ifstream& file, json::json& submsg, std::vector<std::vector<std::string>>& too_long_submessages)
{
  auto row = submsg["loc"]["row_beg"].get<std::size_t>();
  auto row_s = std::to_string(row);

  {
  std::string to_print;

  std::size_t width = 5;
  to_print = to_print + box_top_bot;
  to_print += fmt::format(fmt::bg(fmt::color::dark_magenta), " {} ", row_s) + "  ";
  width += row_s.size();

  // FILE
  std::string tmpbuf,tmpbuf2;
  for(std::size_t i = 0; i < row; ++i)
    std::getline(file, tmpbuf);
  to_print += fmt::format(fmt::bg(fmt::color::gray), tmpbuf);
  to_print + fmt::format(fmt::bg(fmt::color::black), "");
  width += tmpbuf.size();

  if(line_width > width + 1)
    for(std::size_t i = 0; i < line_width - width; ++i)
      to_print += " ";

  to_print = to_print + box_topb_botb + "\n";
  fmt::print(to_print);
  }
  std::size_t mid = 0;
  {
  std::string to_print;

  std::size_t width = 3;
  to_print = to_print + box_top_right + box_left_right + box_left_right;
  for(auto& x : row_s)
    to_print += box_left_right;
  to_print = to_print + box_bot_left + " ";
  width += 2 + row_s.size();

  auto beg = submsg["loc"]["col_beg"].get<std::size_t>();
  auto end = submsg["loc"]["col_end"].get<std::size_t>();
  mid = (end - beg) / 2 + beg;
  assert(beg < end && "Message mark must be a valid interval.");

  // TODO: what if end - beg > line_width
  for(std::size_t i = 1; i < beg; ++i)
    to_print += " ";
  width += (beg - 1);
  // underline
  for(std::size_t i = beg; i < end; ++i)
    to_print += (i != mid ? underline : underline_mid);
  width += (end - beg);

  if(line_width > width - 1)
    for(std::size_t i = 0; i < line_width - width; ++i)
      to_print += " ";

  to_print += box_topb_botb;
  to_print += "\n";
  fmt::print(to_print);
  }
  {
  std::string to_print;

  std::size_t width = 3;
  to_print = to_print + "   ";
  for(auto& x : row_s)
    to_print += " ";
  to_print += box_top_bot;
  width += row_s.size();

  for(std::size_t i = 0; i < mid; ++i,++width)
    to_print += " ";
  to_print = to_print + box_topb_rightb + box_leftb_rightb + " ";
  width += 3;

  // potentially split message based on left-over width
  auto splitted = split_message(width, submsg["msg"].get<std::string>(), line_width - 1);
  if(splitted.empty())
  {
    auto fresh_split = split_message(5 + row_s.size(), submsg["msg"].get<std::string>(), line_width - 1);

    // TODO: fix this assertion by splitting at non-whitespace...?
    assert(!fresh_split.empty() && "If this is empty, the message has a word that is longer than the maximum allowed length.");

    width += 2;
    to_print += submessage_pointers[too_long_submessages.size() % submessage_pointers.size()];

    too_long_submessages.emplace_back(fresh_split);

    for(std::size_t i = 0; i < line_width - width - 1; ++i)
      to_print += " ";
  }
  for(auto it = splitted.begin(); it != splitted.end(); ++it)
  {
    if(it != splitted.begin())
    {
      // first, close current line and print it
      to_print += box_topb_botb;
      to_print += "\n";
      fmt::print(to_print);

      // Now, start again before printing the new thing.
      to_print = "   ";
      for(auto& x : row_s)
        to_print += " ";
      to_print += box_top_bot;
      for(std::size_t i = 0; i < mid; ++i)
        to_print += " ";
      to_print = to_print + "   ";
      // same width now
    }
    auto msg = *it;
    to_print += msg;
    for(std::size_t i = 0; i < line_width - width - msg.size() - 1; ++i)
      to_print += " ";
  }
  to_print += box_topb_botb;
  to_print += "\n";
  fmt::print(to_print);
  }
}

void print_continued_messages(std::size_t line_num_width, std::vector<std::vector<std::string>>& too_long_submessages)
{
  assert(!too_long_submessages.empty() && "Must not be empty.");
  for(std::size_t idx = 0; idx < too_long_submessages.size(); ++idx)
  {
    auto& symbol = submessage_pointers[idx % submessage_pointers.size()];
    auto& msgs = too_long_submessages[idx];

    {
    std::string to_print;

    std::size_t width = line_num_width + 3;
    for(std::size_t i = 0; i < width; ++i)
      to_print += " ";
    to_print = to_print + box_top_bot + " - " + symbol + " ";
    width += 1 + 3 + 2 + 1;

    // msgs will contain at least one thing
    assert(!msgs.empty() && "msgs cannot be empty");
    for(auto it = msgs.begin(); it != msgs.end(); ++it)
    {
      if(it != msgs.begin())
      {
        // add padding
        to_print = "";
        for(std::size_t i = 0; i < line_num_width + 3; ++i)
          to_print += " ";
        to_print = to_print + box_top_bot + "      ";
      }
      to_print += *it;

      for(std::size_t i = 0; i < line_width - width - it->size(); ++i)
        to_print += " ";
      to_print += box_topb_botb;
      to_print += "\n";

      fmt::print(to_print);
    }
    }
  }
}

void print_file_sep_end(std::size_t line_num_width)
{
  std::string to_print;

  std::size_t width = line_num_width + 3;
  for(std::size_t i = 0; i < width; ++i)
    to_print += " ";

  to_print = to_print + box_top_bot + " " + submsg_separator;
  width += 2 + std::string(submsg_separator).size();
  for(std::size_t i = 0; i < line_width - width; ++i)
    to_print += " ";

  to_print = to_print + box_topb_botb + "\n";
  fmt::print(to_print);
}

void print_box_sep(std::size_t line_num_width, std::string with_text)
{
  std::string to_print;

  for(std::size_t i = 0; i < line_num_width + 3; ++i)
    to_print += " ";
  to_print = to_print + box_top_bot_right + box_left_right + " " + with_text;
  for(std::size_t i = 0; i < line_width - line_num_width - 2 - 5 - with_text.size(); ++i)
    to_print += " ";
  to_print = to_print + box_left_right + box_topb_botb_left;

  to_print += "\n";
  fmt::print(to_print);
}

void print_top_box(std::size_t line_num_width, json::json& msg)
{
  const std::string kind = msg["kind"].get<std::string>();
  const std::size_t code = msg["code"].get<std::size_t>();
  const std::string file = msg["file"].get<std::string>();

  std::string to_print;
  for(std::size_t i = 0; i < line_num_width + 3; ++i)
    to_print += " ";

  to_print += box_bot_right;
  to_print = to_print +  box_left_right + " ";
  
  std::size_t width = line_num_width + 3 + 1 + 1 + 1;
  if(kind == "Error")
    to_print += fmt::format(fmt::emphasis::bold | fmt::fg(fmt::color::red), kind);
  else if(kind == "Warning")
    to_print += fmt::format(fmt::emphasis::bold | fmt::fg(fmt::color::white), kind);
  else
    to_print += kind;
  to_print = to_print + ": " + fmt::format(fmt::emphasis::bold, "{} ", code);
  width += kind.size() + 2 + std::to_string(code).size();

  for(std::size_t i = 0; i < line_width - width - 1; ++i)
    to_print += box_left_right;
  to_print += box_botb_left;
  to_print += "\n";
  fmt::print(to_print);

  to_print = "";
  width = line_num_width + 3;

  for(std::size_t i = 0; i < line_num_width + 3; ++i)
    to_print += " ";
  to_print = to_print + box_top_bot + " <";
  to_print += file + ">";
  width += 1 + file.size();

  for(std::size_t i = 0; i < line_width - width - line_num_width - 2; ++i)
    to_print += " ";
  to_print += box_topb_botb;

  to_print += "\n";
  fmt::print(to_print);
}

void print_wiki_line(std::size_t line_num_width, json::json& msg)
{
  const std::string wiki = " Read the wiki: ";
  std::string kind = msg["kind"].get<std::string>();
  const std::size_t code = msg["code"].get<std::size_t>();

  std::transform(kind.begin(), kind.end(), kind.begin(), [](unsigned char c) { return std::tolower(c); });

  std::string to_print;
  std::size_t width = 6 + wiki.size() + std::string(base_url).size() + kind.size() + 1 + std::to_string(code).size();

  for(std::size_t i = 0; i < line_num_width + 3; ++i)
    to_print += " ";
  width -= line_num_width + 2;

  to_print = to_print + box_top_bot + " ";
  std::string complete_url = base_url + kind + "/" + std::to_string(code);

  // compute offset to center the text in the box
  std::size_t whitespaces = (line_width - width - complete_url.size() / 2) / 2;
  for(std::size_t i = 0; i < whitespaces - 1; ++i)
    to_print += " ";
  width -= whitespaces;

  to_print = to_print + fmt::format(fmt::bg(fmt::color::dark_olive_green), "{} Read the wiki: {} ", pointer, complete_url);

  bool at_least_one_whitespace = false;
  for(std::size_t i = 0; i < width - 1; ++i)
  {
    to_print += " ";
    at_least_one_whitespace = true;
  }
  assert(at_least_one_whitespace && "Line width is too narrow to account for the padding");
  to_print += box_topb_botb;

  to_print += "\n";
  fmt::print(to_print);
}

void print_bot_box(std::size_t line_num_width)
{
  std::string to_print;
  for(std::size_t i = 0; i < line_num_width + 3; ++i)
    to_print += " ";
  to_print += box_top_rightb;
  for(std::size_t i = 0; i < line_width - 2 - line_num_width - 2; ++i)
    to_print += box_leftb_rightb;
  to_print += box_topb_leftb;

  to_print += "\n";
  fmt::print(to_print);
}


}

