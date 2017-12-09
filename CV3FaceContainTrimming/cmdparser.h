
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>

class CmdParser {
private:
    struct ARGUMENT {
        std::string LongOpt;
        char ShortOpt;
        std::string Description;
        bool HasValue;
        std::string ArgValue;
    };
    
public:
    
    CmdParser() = default;
    ~CmdParser() = default;
    
    void Add(
        const std::string &longOpt, char shortOpt,
        const std::string &description, bool hasValue=false,
        const std::string &defaultValue="")
    {
        ARGUMENT opt;
        opt.LongOpt = longOpt;
        opt.ShortOpt = shortOpt;
        opt.Description = description;
        opt.HasValue = hasValue;
        
        if( opt.HasValue ) {
            opt.ArgValue = defaultValue;
        } else {
            opt.ArgValue = "false";
        }
        
        vecOptions.push_back(opt);
        
    }
    
    void Parse(int argc, char *argv[]) {
        for( int i = 1; i < argc; i++ ) {
            std::string arg = argv[i];
            
            std::vector<ARGUMENT>::iterator it;
            
            if( arg[0] == '-' ) {
                if( arg[1] == '-' ) {
                    // long option
                    it = std::find_if(
                        std::begin(vecOptions), std::end(vecOptions),
                        [&arg](const ARGUMENT &argOpt) {
                            return argOpt.LongOpt == arg.substr(2);
                        }
                    );
                } else {
                    // short option
                    it = std::find_if(
                        std::begin(vecOptions), std::end(vecOptions),
                        [&arg](const ARGUMENT &argOpt) {
                            return argOpt.ShortOpt == arg[1];
                        }
                    );
                }
            } else {
                std::stringstream ss;
                ss << "Invalid option: " << arg;
                throw std::exception(ss.str().c_str());
            }
            
            if( it == vecOptions.end() ) {
                std::stringstream ss;
                ss << "Unknown option: " << arg;
                throw std::exception(ss.str().c_str());
                
            } else {
                if( it->HasValue ) {
                    if( argc <= i + 1 ) {
                        throw std::exception("Too less argument count");
                    } else {
                        it->ArgValue = argv[++i];
                    }
                } else {
                    it->ArgValue = "true";
                }
            }
            
        }
    }
    
    template <class T> T Get(const std::string &longOpt) const {
        auto it = std::find_if(
            std::begin(vecOptions), std::end(vecOptions),
            [longOpt](const ARGUMENT &arg) {
                return longOpt == arg.LongOpt;
            }
        );
        
        if( it == vecOptions.end() ) {
            throw std::exception("Not found option");
        } else {
            return CastType<T>(it->ArgValue);
        }
        
    }
    
    std::string GetHelp() const {
        std::stringstream ss;
        ss << "commnad line help\n";
        for( const auto &opt : vecOptions ) {
            ss << "  --" << opt.LongOpt << ", -" << opt.ShortOpt << ": "
                << opt.Description << "\n";
        }
        return ss.str();
    }
    
private:
    template <class T> static T CastType(const std::string &val) {
        throw std::exception("Unsupprted type");
        return 0;
    }
    
    template <> static int CastType<int>(const std::string &val) {
        return std::stoi(val);
    }
    template <> static float CastType<float>(const std::string &val) {
        return std::stof(val);
    }
    template <> static double CastType<double>(const std::string &val) {
        return std::stod(val);
    }
    template <> static bool CastType<bool>(const std::string &val) {
        if( val == "true" || val == "1" )
            return true;
        else if( val == "false" || val == "0" )
            return false;
        else
            throw std::exception("Invalid bool cast");
    }
    template <> static std::string CastType<std::string>(const std::string &val) {
        return val;
    }
    
private:
    std::vector<ARGUMENT> vecOptions;
};
