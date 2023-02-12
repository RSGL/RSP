/*
* Copyright (c) 2023 ColleagueRiley ColleagueRiley@gmail.com
*
* This software is provided 'as-is', without any express or implied
* warranty.  In no event will the authors be held liable for any damages
* arising from the use of this software.
*
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
*
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 3. This notice may not be removed or altered from any source distribution.
*/

#include <string>
#include <vector> 
#include <map>
#include <algorithm>

namespace RSP
{
  enum format
  {
    XML,
    HTML,
    SVG,
    JSON,
    GUESS
  }; // format of data, or let the library guess what the proper format is
  enum tokenType{
    open,
    close,
    key,
    value,
    content
  }; // token types for tokenizing

  struct token{
    tokenType t;                             // type
    std::string data;                        // data the token holds
    std::map<std::string, std::string> args; // args
  };                                         // token objects for tokenizing

  struct data{
    std::string key;   // key (for this data index)
    std::string value; // value (for this data index)

    std::vector<data> next; // source vector
    std::map<std::string, std::string> args; // arguments (for this data index)

    data* prev = NULL;

    // the stuff you actual want from as a user
    void push(std::string key, std::string value); // push key/value
    void push(data d){ next.push_back(d); }
    void pop(){ next.pop_back(); }
    bool empty() { return next.empty(); };
    int size()  { return next.size(); };

    data& operator[](std::string key); // [] function
  };                                   // data format object (for user)

  std::vector<std::string> voidTags;

  data loadF(std::string file, format c = GUESS); // load data from file
  data loadS(std::string data, format c = GUESS); // load data from string

  void writeF(std::string file, data d, format c); // load data from file
  std::string writeS(data d, format ); // load data from string

  // these functions are run by the load functions
  std::vector<token> tokenizeXML(std::string data, format c); // tokenize xml data
  data parseXML(std::vector<token> tokens, format c);         // parse xml tokens

  std::vector<token> tokenizeJSON(std::string data); // tokenize json data
  data parseJSON(std::vector<token> tokens);         // parse json data
}

#ifdef RSP_IMPLEMENTATION

RSP::data out;

RSP::data &RSP::data::operator[](std::string key){        // [] function source
  int i; // index
  for (i = 0; i < next.size() && next[i].key != key; i++); // find the index of the key

  if (next[i].key != key){
    printf("RSP::data :: Key not found \"");
    printf(key.c_str());
    printf("\"");

    return out;
  }

  return next[i]; // return the srcs that holds the same key
}

void RSP::data::push(std::string key, std::string value){
  next.push_back({key, value});
}

std::vector<RSP::token> RSP::tokenizeXML(std::string data, format c){
  if (c == HTML)
    voidTags = {"area" , "base" , "br" , "col" , "command" , "embed" , "hr" , "img" , "input" , "keygen" , "link" , "meta" , "param" , "source" , "track" , "wbr"};

  std::vector<RSP::token> tokens;

  for (int i = 0; i < data.size(); i++){
    switch (data[i])
    {
    case '<': { // if the data is a <, let's check it
      if (data[i + 1] != '!')
      {                                                // if it has a ! after the <, it's a comment tag, so only check it if it's not a comment
        token t = {data[i + 1] != '/' ? open : close}; // if there is a / after the <, it's a close tag, else it's an open tag

        // get the tag's name
        t.data = data;

        t.data.replace(
            t.data.begin(),
            t.data.begin() + i + ((data[i + 1] != '/') ? 1 : 2), // if it has a / after, replace an extra character
            "");                                                 // delete all the text up to the name

        int newI = t.data.find_first_of(">") + ((data[i + 1] != '/') ? 1 : 2) + i;

        t.data.replace(t.data.begin() + t.data.find_first_of(">"), t.data.end(), ""); // delete everything after the tag closes (gets the name if there are no args)
        
        int dist = std::distance(voidTags.begin(), std::find(voidTags.begin(), voidTags.end(), t.data));
        bool isVoid = (dist < t.data.size());

        // check if there is a space and thereby, if there will be args
        if (t.data.find_first_of(' ') < t.data.size()){
          t.data.replace(t.data.begin() + t.data.find_first_of(' '), t.data.end(), ""); // delete everything after the space to get the name

          tokens.push_back(t); // push token into token data

          // start collecting args
          int nameS = t.data.size();  // size of the tag name
          std::string argData = data; // data for searching for

          argData.replace(
              argData.begin(),
              argData.begin() + i + ((data[i + 1] != '/') ? 1 : 2) + nameS + 1, // if it has a / after, replace an extra character, replace up to args + space
              "");                                                              // delete all the text up to the args

          argData.replace(argData.begin() + argData.find_first_of(">"), argData.end(), ""); // delete everything after the tag closes to get just the args

          for (int j = 0; j < argData.size(); j++){
            if (argData[j] == '=')
            { // we found an arg
              // get the name
              t = {key, argData};

              t.data.replace(t.data.begin() + j, t.data.end(), ""); // delete everything extra ahead of the arg's name

              tokens.push_back(t); // push the key

              t.data.replace(t.data.begin(), t.data.begin() + t.data.find_first_not_of(' '), "");
              
              // get the value
              t = {value, argData};
              t.data.replace(t.data.begin(), t.data.begin() + j + 1, ""); // delete everything in front the value
              
              if (t.data[0] != '\"'){
                if (t.data.find_first_of(' ') < t.data.size())                                  // if there's args after
                  t.data.replace(t.data.begin() + t.data.find_first_of(' '), t.data.end(), ""); // delete everything in bhind the value
              }

              else{
                if (t.data.find_first_of('\"', 1) < t.data.size())                                // if there's args after
                  t.data.replace(t.data.begin() + t.data.find_first_of('\"', 1) + 1, t.data.end(), ""); // delete everything in bhind the value
              }

              t.data.replace(t.data.begin(), t.data.begin() + t.data.find_first_not_of(' '), "");

              tokens.push_back(t); // push the value

              int argSize = t.data.size();

              argData.replace(argData.begin(), argData.begin() + j + 1, "");
              argData.replace(argData.begin(), argData.begin() + argSize + 1, "");

              j = 0;
            }
          }
        }

        else if (t.t != close || (t.t == close && !isVoid)){
          tokens.push_back(t); // push token into token data
        }

        if (isVoid){
          t.t = close;
          tokens.push_back(t);
        }

        i = newI;

        if (data[i - 1] == '/')
          tokens.push_back({close});
      }
      else
        i += data.find_first_of(">", i) + 1; // skip after tag
      break;
    }

    default:
      token t = {content, data};
      t.data.replace(t.data.begin(), t.data.begin() + i, ""); // replace up to the index

      if (t.data.find_first_of('<') < t.data.size()){
        t.data.replace(t.data.begin() + t.data.find_first_of('<'), t.data.end(), ""); // replace after the tag open

        i += t.data.find_first_of('<') + t.data.size(); // set index to where next open tag is
      }

      tokens.push_back(t); // send content tag
      break;
    }
  }

  return tokens;
}

std::vector<RSP::token> RSP::tokenizeJSON(std::string data)
{
  std::vector<RSP::token> tokens;

  for (int i = 0; i < data.size(); i++)
  {
    switch (data[i])
    {
    case '{':
      tokens.push_back({open});
      break;
    case ':': {
      token t = {key, data};

      t.data.replace(t.data.begin() + i, t.data.end(), "");
      std::reverse(t.data.begin(), t.data.end());

      t.data.replace(t.data.begin(), t.data.begin() + t.data.find_first_of('\"') + 1, "");

      t.data.replace(t.data.begin() + t.data.find_first_of('\"'), t.data.end(), "");

      std::reverse(t.data.begin(), t.data.end());

      tokens.push_back(t);

      t = {value, data};

      t.data.replace(t.data.begin(), t.data.begin() + i + 1, "");

      if (t.data.find_first_of(',') < t.data.find_first_of('}'))
        t.data.replace(t.data.begin() + t.data.find_first_of(','), t.data.end(), "");
      else
        t.data.replace(t.data.begin() + t.data.find_first_of('}'), t.data.end(), "");
    
      t.data.replace(t.data.begin(), t.data.begin() + t.data.find_first_not_of(' '), "");

      if (t.data.size() && t.data[0] != '{'){
        tokens.push_back(t);
        i += t.data.size();
      }
      break;
    }
    case '}':
      tokens.push_back({close});
      break;
    }
  }

  return tokens;
}

RSP::data RSP::parseJSON(std::vector<RSP::token> tokens){
  RSP::data index;
  std::vector<RSP::data> prev;

  std::string curArg;

  for (auto &t : tokens){
    switch (t.t){
      case open:
        index.push(curArg, "");
        
        prev.push_back(index);
        index = index.next.back();
        break;
      case close:
        if (prev.size()){
          prev.back().next.back() = index;

          index = prev.back();
          
          prev.pop_back();
        }
        break;
      case key:
        curArg = t.data;
        break;
      case value:
        index.push(curArg, t.data);
        break;
    }
  }
  
  
  return index[""];
}

RSP::data RSP::parseXML(std::vector<RSP::token> tokens, RSP::format c){
  RSP::data index;
  std::vector<RSP::data> prev;

  std::string curArg;

  for (auto &t : tokens){
    switch (t.t){
      case open:
        if (!index.key.empty()){
          index.push(t.data, "");
          
          prev.push_back(index);

          index = index.next.back();
        }
        else 
          index.key = t.data;
        break;
      case close:
        if (prev.size()){
          prev.back().next.back() = index;

          index = prev.back();

          prev.pop_back();
        }
        break;
      case key:
        t.data.replace(t.data.begin(), t.data.begin() + t.data.find_first_not_of(' '), "");
        curArg = t.data;

        break;
      case value:
        index.args.insert({curArg, t.data});
        break;
      case content:
        index.value = t.data;
        break;
    }
  }
  
  
  return index;
}

RSP::data RSP::loadF(std::string file, RSP::format c){
  FILE *f = fopen(file.c_str(), "r");

  fseek(f, 0L, SEEK_END);

  const int size = ftell(f);

  fseek(f, 0L, SEEK_SET);

  char fData[size];

  fread(fData, 1, size, f);

  fclose(f);

  return loadS(fData);
}

RSP::data RSP::loadS(std::string data, RSP::format c)
{
  if (c == GUESS)
  {
    if (data.find_first_of('<') > data.find_first_of('{'))
      c = JSON;

    else if (data.find_first_of("html") < data.size())
      c = HTML;
    else if (data.find_first_not_of("svg") < data.size())
      c = SVG;
    else
      c = XML;
  }

  if (c == SVG || c == XML || c == HTML)
    return parseXML(tokenizeXML(data, c), c);
  else
    return parseJSON(tokenizeJSON(data));
}

void RSP::writeF(std::string file, RSP::data d, RSP::format c){
  std::string dStr = writeS(d, c);
  
  FILE *f = fopen(file.c_str(), "w+");

  fwrite(dStr.c_str(), 1, dStr.size(), f);

  fclose(f);
}

void RSPjsonStr(RSP::data d, std::string& str){
  int i = 0;

  for (auto& n : d.next){
    i++;

    if (n.key[0] != '\"'){
      n.key.insert(n.key.begin(), '\"');

      n.key += "\"";
    }
    
    if (n.next.size()){
      str += "\n\n" + n.key + " : " + " {";
      RSPjsonStr(n, str);
      str += "\n}";
    }

    else
      str += "\n" + n.key + ":" + n.value;

    if (i < d.next.size())
      str += ",";
  }
}


void RSPXMLStr(RSP::data d, std::string& str){
  for (auto& n : d.next){
    str += "<" + n.key;

    for (auto& a : n.args)
      str += " " + a.first + "=" + a.second;

    str += ">\n";

    str += n.value + "\n";

    if (n.next.size()){
      str += "\n";
      RSPXMLStr(n, str);
    }

    str += "</" + n.key + ">\n";
  }
}

std::string RSP::writeS(RSP::data d, RSP::format c){
  std::string output;

  if (c == JSON){
    output += "{";
    RSPjsonStr(d, output);
    output += "\n\n}";
  }

  else if (c == HTML || c == XML || c == SVG){
    output += "<!DOCTYPE " + (std::string)((c == HTML) ? "html" : (c == XML) ? "xml" : "svg") + ">\n";

    RSPXMLStr(d, output);
  }

  return output;
}

#endif