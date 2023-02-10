#include <string>
#include <vector>
#include <map>
#include <algorithm>

namespace RSP{
    enum format { XML, HTML, SVG, JSON, GUESS }; // format of data, or let the library guess what the proper format is
    enum tokenType { open, close, key, value, content }; // token types for tokenizing
    
    struct token{
        tokenType t; // type
        std::string data; // data the token holds
        std::map<std::string, std::string> args; // args
    }; // token objects for tokenizing

    struct data{
      std::string key; // key (for this data index)
      std::string value; // value (for this data index)
      std::string content;
      std::map<std::string, std::string> args; // arguments (for this data index)

      std::vector<data> src; // source vector

      // the stuff you actual want from as a user

      void push(std::string key, std::string value); // push key/value
      
      data& operator[](std::string key); // [] function
    }; // data format object (for user)

    data loadF(std::string file, format c = GUESS); // load data from file
    data loadS(std::string data, format c = GUESS); // load data from string

    // these functions are run by the load functions 
    std::vector<token> tokenizeXML(std::string data, format c); // tokenize xml data 
    data parseXML(std::vector<token> tokens, format c);  // parse xml tokens

    std::vector<token> tokenizeJSON(std::string data); // tokenize json data
    data parseJSON(std::vector<token> tokens); // parse json data
}

#define RSP_IMPLEMENTATION
#include <iostream>
#warning Delete these

#ifdef RSP_IMPLEMENTATION

RSP::data& RSP::data::operator[](std::string key){ // [] function source
  int i; // index
  for (i = 0; i < src.size() && src[i].key != key; i++); // find the index of the key

  return src[i]; // return the srcs that holds the same key
}

void RSP::data::push(std::string key, std::string value){
    src.push_back({key, value}); 
}

std::vector<RSP::token> RSP::tokenizeXML(std::string data, format c){
    std::vector<RSP::token> tokens;

    for (int i = 0; i < data.size(); i++){
        switch (data[i]){ 
            case '<': { // if the data is a <, let's check it
                if (data[i + 1] != '!'){ // if it has a ! after the <, it's a comment tag, so only check it if it's not a comment
                    token t = { data[i + 1] != '/' ? open : close}; // if there is a / after the <, it's a close tag, else it's an open tag
                    
                    // get the tag's name
                    t.data = data; 

                    t.data.replace(
                        t.data.begin(), 
                        t.data.begin() + i + ((data[i + 1] != '/') ? 1 : 2),  // if it has a / after, replace an extra character 
                        ""); // delete all the text up to the name

                    int newI = t.data.find_first_of(">") + ((data[i + 1] != '/') ? 1 : 2) + i;
                    
                    t.data.replace(t.data.begin() + t.data.find_first_of(">"), t.data.end(), ""); // delete everything after the tag closes (gets the name if there are no args)

                    // check if there is a space and thereby, if there will be args
                    if (t.data.find_first_of(' ') < t.data.size()){
                        t.data.replace(t.data.begin() + t.data.find_first_of(' '), t.data.end(), ""); // delete everything after the space to get the name
                      
                        tokens.push_back(t); // push token into token data

                        // start collecting args
                        int nameS = t.data.size(); // size of the tag name
                        std::string argData = data; // data for searching for
                        
                        argData.replace(
                          argData.begin(), 
                          argData.begin() + i + ((data[i + 1] != '/') ? 1 : 2) + nameS + 1,  // if it has a / after, replace an extra character, replace up to args + space
                          ""
                        ); // delete all the text up to the args

                        argData.replace(argData.begin() + argData.find_first_of(">"), argData.end(), ""); // delete everything after the tag closes to get just the args

                        for (int j = 0; j < argData.size(); j++){
                          if (argData[j] == '='){ // we found an arg
                            // get the name
                            t = {key, argData};
                                
                            t.data.replace(t.data.begin() + j, t.data.end(), ""); // delete everything extra ahead of the arg's name
                            t.data.replace(t.data.begin(), t.data.begin() + t.data.find_last_of(' ') + 1, ""); // delete everything extra behind the arg's name

                            tokens.push_back(t); // push the key
                            
                            // get the value
                            t = {value, argData};
                            t.data.replace(t.data.begin(), t.data.begin() + j + 1, ""); // delete everything in front the value
                            
                            if (t.data.find_first_of(' ') < t.data.size()) // if there's args after 
                              t.data.replace(t.data.begin() + t.data.find_first_of(' '), t.data.end(), ""); // delete everything in bhind the value
                            
                            tokens.push_back(t); // push the value
                          }
                        }
                    } 
                    
                    else 
                      tokens.push_back(t); // push token into token data

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

std::vector<RSP::token> RSP::tokenizeJSON(std::string data){
  std::vector<RSP::token> tokens;
  
  for (int i = 0; i < data.size(); i++){
    switch (data[i]){
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
        
        while (t.data[0] == ' ')
            t.data.pop_back();

        if (t.data.size()) 
          tokens.push_back(t);

        i += t.data.size();
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
  data d;
  data& c = d;
  
  for (int i = 0; i < tokens.size(); i++){
    switch (tokens[i].t){
      case open: 
          std::cout << "open : "; 
          c[tokens[i].data];
          break;
      case key: 
          std::cout << "key : "; break;
      case value: 
          std::cout << "value : "; break;
      case close: 
          std::cout << "close : "; break;
    }
    
    std::cout << tokens[i].data << std::endl;
  }    

  return d;
}

RSP::data RSP::parseXML(std::vector<RSP::token> tokens, RSP::format c){
    RSP::data index;

    for (auto& t : tokens){
      switch (t.t){
        case open: std::cout << "open : "; break;
        case close: std::cout << "close : "; break;
        case key: std::cout << "key : "; break;
        case value: std::cout << "value : "; break;
        case content: std::cout << "content : "; break;
      }

      std::cout << t.data << std::endl;
    }

    return index;
}

RSP::data RSP::loadF(std::string file, RSP::format c){
    FILE* f = fopen(file.c_str(), "r");

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

#endif