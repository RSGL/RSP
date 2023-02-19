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

#include <string> // std::string
#include <vector> // std::vector
#include <map> // std::map
#include <algorithm> // algorithm functions, std::find, std::reverse, ect

namespace RSP {
  enum format {
    XML,
    HTML,
    SVG,
    JSON,
    CSV_COMMA, // CSV using , as a divider
    CSV_SEMI, // CSV using ; as a divider
    CSV_GUESS, // CSV guess the divider
    GUESS // Tell the function to guess what format the data is
  }; // format of data, or let the library guess what the proper format is

  enum tokenType {
    open, // open braces or tag
    close, // close braces or tag
    openList, // open list [
    closeList, // close list ]
    key, // JSON key, collumn name, argument name
    value, // Value of key, value in list, value of argument
    content // Content between tag open/close
  }; // token types for tokenizing

  struct token {
    tokenType t;                             // type
    std::string data;                        // data the token holds
    std::map<std::string, std::string> args; // args (XML)
  };                                         // token objects for tokenizing

  struct data {
    std::string key;        // key (for this data index)
    std::string value;      // value (for this data index) (content of tag for XML)
    std::vector<data> list; // list value, if the value is a list (for JSON), (src list for csv)

    std::vector<data> next;                  // next data, if there is any
    std::map<std::string, std::string> args; // arguments (for this data index) (XML only)

    void push(std::string key, std::string value) { next.push_back({key, value}); } // push key/value to next
    void push(data d) { next.push_back(d); }                                        // push data object to next
    void pop() { next.pop_back(); }                                                 // remove last object of next
    bool empty() { return next.empty(); };                                          // returns true if next is empty
    int size() { return next.size(); };                                             // returns the size of next

    data &operator[](std::string key);                  // [] function
    data &operator[](int index) { return list[index]; } // [] function for lists
  };                                                    // data format object (for user)

  std::vector<std::string> voidTags; // void tags (for HTML)

  data loadF(std::string file, format c = GUESS); // load data from file
  data loadS(std::string data, format c = GUESS); // load data from string

  void dumpF(std::string file, data d, format c); // dump data into a file
  std::string dumpF(data d, format);              // dump data into a string

  // these functions are run by the load functions
  std::vector<token> tokenizeXML(std::string data, format c); // tokenize xml data
  data parseXML(std::vector<token> tokens, format c);         // parse xml tokens

  std::vector<token> tokenizeJSON(std::string data); // tokenize json data
  data parseJSON(std::vector<token> tokens);         // parse json data

  std::vector<token> tokenizeCSV(std::string data, format c = CSV_GUESS); // tokenize csv data
  data parseCSV(std::vector<token> tokens);         // parse csv data
}

#ifdef RSP_IMPLEMENTATION

RSP::data error = {"RSP-ERROR"}; // error data obj to output in case of errors

RSP::data &RSP::data::operator[](std::string key) {        // [] function source
  int i; // index
  for (i = 0; i < next.size() && next[i].key != key; i++); // find the index of the key

  if (next[i].key != key) { // the key was not found
    #ifndef RSP_QUIET_ERRORS    
    printf("RSP::data :: Key not found \"%s\"\n", key.c_str()); // print error
    #endif

    error.value = "Key not found"; // the actual error (for checking)

    return error; // return blank obj
  }

  return next[i]; // return the srcs that holds the same key
}

std::vector<RSP::token> RSP::tokenizeXML(std::string data, format c){
  if (c == HTML) // if we're using HTML, fill voidTags with HTML's void tags
    voidTags = {"area", "base", "br", "col", "command", "embed", "hr", "img", "input", "keygen", "link", "meta", "param", "source", "track", "wbr"};

  std::vector<RSP::token> tokens; // output tokens

  for (int i = 0; i < data.size(); i++)
  {
    switch (data[i])
    {
    case '<':
    { // if the data is a <, let's check it
      if (data[i + 1] != '!'){                                                // if it has a ! after the <, it's a comment tag, so only check it if it's not a comment
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
        if (t.data.find_first_of(' ') < t.data.size())
        {
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

          for (int j = 0; j < argData.size(); j++)
          {
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

              if (t.data[0] != '\"')
              {
                if (t.data.find_first_of(' ') < t.data.size())                                  // if there's args after
                  t.data.replace(t.data.begin() + t.data.find_first_of(' '), t.data.end(), ""); // delete everything in bhind the value
              }

              else
              {
                if (t.data.find_first_of('\"', 1) < t.data.size())                                      // if there's args after
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

        else if (t.t != close || (t.t == close && !isVoid))
        {
          tokens.push_back(t); // push token into token data
        }

        if (isVoid)
        {
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

      if (t.data.find_first_of('<') < t.data.size())
      {
        t.data.replace(t.data.begin() + t.data.find_first_of('<'), t.data.end(), ""); // replace after the tag open

        i += t.data.find_first_of('<') + t.data.size(); // set index to where next open tag is
      }

      tokens.push_back(t); // send content tag
      break;
    }
  }

  return tokens;
}

std::vector<RSP::token> RSP::tokenizeJSON(std::string data){
  std::vector<RSP::token> tokens;
  int list = 0;

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

      if (t.data.size() && t.data[0] != '{' && t.data[0] != '[')
      {
        tokens.push_back(t);
        i += t.data.size();
      }
      break;
    }
    case '[': {
        tokens.push_back({openList});
        std::string lData = data;

        lData.replace(lData.begin(), lData.begin() + i + 1, "");
        
        int list = 1;

        for (i = i + 1; i < data.size() && list; i++){
          if (data[i] == '[')
            list++;
          
          else if (data[i] == ']')
            list--;
        }

        i--;
        list = 1;

        for (int j = 0; j < lData.size(); j++){
          lData.replace(lData.begin(), lData.begin() + lData.find_first_not_of(' '), "");
          
          token t = {value, lData};

          char c = lData[0];

          if (c == '[')
            list++;

          else if (c == ']')
            list--;
          
          if (!list)
            break;

          if (c == '[' || c == '{'){
            int embed = 1;
            for (j = 1; j < lData.size() && embed; j++){
              if (lData[j] == c){
                embed++;
              }
              
              if ((lData[j] == ']' && c == '[') || (lData[j] == '}' && c == '{'))
                embed--;
            }

            t.data.replace(t.data.begin() + j, t.data.end(), "");
            
            lData.replace(lData.begin(), lData.begin() + t.data.size(), "");
            lData.replace(lData.begin(), lData.begin() + lData.find_first_not_of(' '), "");    
            
            if (lData[0] == ',')
              lData[0] = ' ';
          }

          else if (t.data.find_first_of(",") < t.data.size() && t.data.find_first_of(",") < t.data.find_first_of(']')){
            t.data.replace(t.data.begin() + t.data.find_first_of(","), t.data.end(), "");

            lData.replace(lData.begin(), lData.begin() + lData.find_first_of(",") + 1, "");
          }
          
          else if (t.data.find_first_of(",") > t.data.find_first_of(']')){
            t.data.replace(t.data.begin() + t.data.find_first_of("]"), t.data.end(), "");

            lData.replace(lData.begin(), lData.begin() + lData.find_first_of("]"), "");         
          }

          tokens.push_back(t);
        }
      } 

      if (data[i] == ']'){
        tokens.push_back({closeList});
        list--;
      }

      break;

    case '}':
      tokens.push_back({close});
      break;
    }
  }

  return tokens;
}

std::vector<RSP::token> RSP::tokenizeCSV(std::string data, RSP::format c) {
  const std::string alph = "abcdefghijklmnopqrstuvwxyz";

  char s = (c == CSV_SEMI) ? ';' : ',';
 
  if (c == CSV_SEMI){
    std::string firstLine = data;

    firstLine.replace(
        firstLine.begin() + firstLine.find_first_of('\n'), 
        firstLine.begin(), 
        "");

    if (firstLine.find(';') < firstLine.size())
        s = ';';
    else if (firstLine.find(',') > firstLine.size())
      return {};
  }

  bool header = true;
  int index = 0;
  std::vector<std::string> keys;
  
  std::string json = "[";

  for (int i = 0; i < data.size(); i++){
    int gotdata = 1;
    
    if (data[i] == s){
      if (header){
          getKey:
            std::string name = data;
            name.replace(name.begin() + i, name.end(), "");
            std::reverse(name.begin(), name.end());
  
            if (name.find_first_of(s) < name.size())
              name.replace(name.begin() + name.find_first_of(s), name.end(), "");
    
            std::reverse(name.begin(), name.end());
            
            if (name[0] != '"'){
                name.insert(name.begin(), '"');

                name += '"';
            }

            keys.push_back(name);
            
            if (gotdata)
              gotdata = 2;
      }
        
      if (!index && !header)
        json += "\n  {";   
      
      if (!header){
        getdata: 
          json += "\n      " + keys[index] + " : ";
          std::string name = data;
          name.replace(name.begin() + i, name.end(), "");
          std::reverse(name.begin(), name.end());
          if (index)
              name.replace(name.begin() + name.find_first_of(s), name.end(), "");
          else
            name.replace(name.begin() + name.find_first_of('\n'), name.end(), "");
  
          std::reverse(name.begin(), name.end());

          if (name[0] != '"'){
            bool hasLetter = false; 
            
            for (auto& c : name)
                if (alph.find(std::tolower(c)) < alph.size()){
                    hasLetter = true;
                    break;
                }

            if (hasLetter){
                name.insert(name.begin(), '"');

                name += '"';
            }
          }
          
          json += name;
        
          if (index < keys.size() - 1)
            json += ",";

          if (gotdata)
            index++;
          else
            gotdata = 2;
      }
    }
    
    if (data[i] == '\n'){
      if (!header){
        if (gotdata == 1){
          gotdata = 0;
          goto getdata;
        }
        json += "\n  },\n";
        index = 0;
      }
   
      else{ 
        if (gotdata == 1){
          gotdata = 0;
          goto getKey;
        }

        header = false;
      }  
    }
  }

  if (json.size() && json[json.size() - 2] == ',')
    json.erase(json.size() - 2);

  json += "\n]";

  return tokenizeJSON(json);
}

RSP::data RSP::parseCSV(std::vector<RSP::token> tokens){ return parseJSON(tokens); }

RSP::data RSP::parseJSON(std::vector<RSP::token> tokens){
  RSP::data index;
  std::vector<RSP::data> prev;

  std::string curArg;

  int list = 0;

  for (auto &t : tokens){
    switch (t.t)
    {
    case open:
      if (!list) {
        index.push(curArg, "");

        prev.push_back(index);
        index = index.next.back();
      }
      break;

    case close:
      if (prev.size() && !list) {
        prev.back().next.back() = index;

        index = prev.back();

        prev.pop_back();
      }
      break;

    case key:
      curArg = t.data;
      break;

    case value:
      std::reverse(t.data.begin(), t.data.end());
      t.data.replace(t.data.begin(), t.data.begin() + t.data.find_first_not_of(' '), "");
      std::reverse(t.data.begin(), t.data.end());

      if (list == false)
        index.push(curArg, t.data);
      else {
        if (t.data[0] == '{' || t.data[0] == '[') {
          if (t.data[0] == '{')
            index.list.push_back(loadS(t.data, JSON));
          else if (t.data[0] == '['){
            index.list.push_back({"", .list = loadS(t.data, JSON).list});
          }
        }
        else
          index.list.push_back({"", t.data});
      }
      break;

    case openList:
      list++;

      index.push(curArg, "");
      prev.push_back(index);
      index = index.next.back();
      break;

    case closeList:
      if (prev.size()) {
        prev.back().next.back() = index;

        index = prev.back();

        prev.pop_back();
      }

      list--;
      break;
    }
  }

  if (index.next.size())
    return index[""];
  else{
    printf("Failed to parse JSON tokens\n");
    return {};
  }
}

RSP::data RSP::parseXML(std::vector<RSP::token> tokens, RSP::format c) {
  RSP::data index;
  std::vector<RSP::data> prev;

  std::string curArg;

  for (auto &t : tokens) {
    switch (t.t)
    {
    case open:
      if (!index.key.empty()) {
        index.push(t.data, "");

        prev.push_back(index);

        index = index.next.back();
      }
      else
        index.key = t.data;
      break;
    case close:
      if (prev.size()) {
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

RSP::data RSP::loadF(std::string file, RSP::format c)
{
  FILE *f = fopen(file.c_str(), "r");

  fseek(f, 0L, SEEK_END);

  const int size = ftell(f);

  fseek(f, 0L, SEEK_SET);

  char fData[size];

  fread(fData, 1, size, f);

  fclose(f);

  return loadS(fData);
}

RSP::data RSP::loadS(std::string data, RSP::format c){
  if (c == GUESS){
    if ((data.find_first_of('<') > data.find_first_of(',') && data.find_first_of('{') > data.find_first_of(',')) ||
        (data.find_first_of('<') > data.find_first_of(';') && data.find_first_of('{') > data.find_first_of(';')))
      c = CSV_GUESS;

    else if (data.find_first_of('<') > data.find_first_of('{'))
      c = JSON;

    else if (data.find_first_of("html") < data.size())
      c = HTML;
    else if (data.find_first_not_of("svg") < data.size())
      c = SVG;
    else
      c = XML;
  }

  while (data.find('\n') < data.size() && (c != CSV_COMMA && c != CSV_SEMI && c != CSV_GUESS))
    data.replace(data.begin() + data.find('\n'), data.begin() + data.find('\n') + 1, "");

  if (c == SVG || c == XML || c == HTML)
    return parseXML(tokenizeXML(data, c), c);
  else if (c == JSON)
    return parseJSON(tokenizeJSON(data));
  else
    return parseCSV(tokenizeCSV(data, c));
}

void RSP::dumpF(std::string file, RSP::data d, RSP::format c) {
  std::string dStr = dumpF(d, c);

  FILE *f = fopen(file.c_str(), "w+");

  fwrite(dStr.c_str(), 1, dStr.size(), f);

  fclose(f);
}

void RSPjsonStr(RSP::data d, std::string &str) {
  int i = 0;

  for (auto &n : d.next)
  {
    i++;

    if (n.key[0] != '\"')
    {
      n.key.insert(n.key.begin(), '\"');

      n.key += "\"";
    }

    if (n.next.size())
    {
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

void RSPXMLStr(RSP::data d, std::string &str)
{
  for (auto &n : d.next)
  {
    str += "<" + n.key;

    for (auto &a : n.args)
      str += " " + a.first + "=" + a.second;

    str += ">\n";

    str += n.value + "\n";

    if (n.next.size())
    {
      str += "\n";
      RSPXMLStr(n, str);
    }

    str += "</" + n.key + ">\n";
  }
}

std::string RSP::dumpF(RSP::data d, RSP::format c)
{
  std::string output;

  if (c == JSON)
  {
    output += "{";
    RSPjsonStr(d, output);
    output += "\n\n}";
  }

  else if (c == HTML || c == XML || c == SVG) {
    output += "<!DOCTYPE " + (std::string)((c == HTML) ? "html" : (c == XML) ? "xml"
                                                                             : "svg") +
              ">\n";

    RSPXMLStr(d, output);
  }

  return output;
}

#endif