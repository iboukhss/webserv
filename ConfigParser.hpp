

/*
What should the task do ?
-> take a path
-> open the config file
-> parse it and feed a struct holding the data


*/

#ifndef CONFIGPARSER_HPP_
#define CONFIGPARSER_HPP_

#include <string>
#include <vector>

/* routing logic :
    - determine which server should handle the request
    - requests without the host should be dropped
*/

class ConfigParser {
public:
    ConfigParser(const std::string& file_path);
    ~ConfigParser();

private:
    std::vector<server_config> config;

    ConfigParser();
};

#endif
