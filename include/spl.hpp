namespace kw
{
   constexpr std::string_view
      COMMENT { "//" },
      SPL { "SPL" },
      FEATURES { "FEATURES" },
      CATEGORIES { "CATEGORIES" },
      F { "F" },
      A { "A" },
      O { "O" },
      N { "N" },
      AN { "AN" },
      ON { "ON" },
      ANF { "ANF" },
      ONF { "ONF" },
      END { "END" },
      EXPLICIT_DEPENDENCIES { "EXPLICIT_DEPENDENCIES" },
      REQUIRES { "REQUIRES" },
      EXCLUDES { "EXCLUDES" },
      MISSING_SYSTEMS { "MISSING_SYSTEMS" },
      AVAILABLE_SYSTEMS { "AVAILABLE_SYSTEMS" };
}

[[nodiscard]] std::string to_upper(std::string s)
{
   for (auto& c : s)
   {
      c = std::toupper(c);
   }
   return s;
}

//using namespace std::literals::string_literals;

class spl_specification_t final
{
   public:
      using feature_names_t = std::vector<std::string>;
      using dependencies_t = std::multimap<unsigned_integer,unsigned_integer>;
      using systems_t = std::set<unsigned_integer>;
      explicit spl_specification_t(const std::string& filename);
      ~spl_specification_t();
      [[nodiscard]] bool setup();
      [[nodiscard]] bool parse();
      void parse_comment();
      void parse_spl();
      void parse_features();
      void parse_categories();
      void parse_explicit_dependencies();
      void parse_missing_systems();
      void parse_available_systems();
      void parse_skip_line();
      void parse_feature();
      void parse_dependency(const std::string& f1,const std::string& dependency, const std::string& f2);
      unsigned_integer feature_index(const std::string& f);
      const std::string m_filename;
      std::ifstream m_file;
      const bool m_success_open_file;
      const bool m_success_parse;
      std::string m_spl { };
      unsigned_integer m_F { };
      feature_names_t m_feature_names;
      unsigned_integer m_categories { F };
      dependencies_t m_requires,
                     m_is_required_by,
                     m_excludes,
                     m_is_excluded_by;
      systems_t m_missing_systems,
                m_available_systems;
      bool m_spl_specified { false },
           m_feature_names_specified { false },
           m_categories_specified { false },
           m_explicit_dependencies_specified { false },
           m_missing_systems_specified { false },
           m_available_systems_specified { false }; 
};

spl_specification_t::spl_specification_t(const std::string& filename):
   m_filename { filename },m_success_open_file { setup() },
   m_success_parse { parse() }
{
}

spl_specification_t::~spl_specification_t()
{
   m_file.close();
}

bool spl_specification_t::setup()
{
   m_file.open(m_filename);
   if (!m_file)
   {
      throw std::runtime_error { std::string { } + "Cannot open file named " +
                                 m_filename + " in " + 
                                 std::source_location::current().function_name() +
                                 " (" + 
                                 std::source_location::current().file_name() + 
                                 ")."
                               };
/*      std::cerr << "Cannot open file named "
                << m_filename << " in "
                << std::source_location::current().function_name() << " (" 
                << std::source_location::current().file_name() << ")."
                << std::endl;
      return false;*/

   }
   return true;
}

bool spl_specification_t::parse()
{
/*   if (!m_success_open_file)
   {
      return false;
   }*/
   std::string token;
   while (m_file)
   {
      m_file >> token;
      if (m_file)
      {
         auto keyword { to_upper(token) };
         if (keyword == kw::COMMENT)
         {
            parse_comment();
         }
         else
         if (keyword ==  kw::SPL)
         {
            parse_spl();
         }
         else
         if (keyword == kw::FEATURES)
         {
            parse_features();
         }
         else
         if (keyword == kw::CATEGORIES)
         {
            parse_categories();
         }
         else
         if (keyword == kw::EXPLICIT_DEPENDENCIES)
         {
            parse_explicit_dependencies();
         }
         else
         if (keyword == kw::MISSING_SYSTEMS)
         {
            parse_missing_systems();
         }
         else
         if (keyword == kw::AVAILABLE_SYSTEMS)
         {
            parse_available_systems();
         }
         else
         {
            throw std::runtime_error { std::string { } + "Unexpected token " +
                                       token + " in file " + m_filename + " in " +
                                       std::source_location::current().function_name() +
                                       " (" + 
                                       std::source_location::current().file_name() + 
                                       ")."
                                     };
                    
         }
      }
   }
   return true;
}

void spl_specification_t::parse_comment()
{
   parse_skip_line();
}

void spl_specification_t::parse_spl()
{
   if (m_spl_specified)
   {
      throw std::runtime_error { std::string { } + "Name of SPL specified more than once in file " + 
                                 m_filename + " in " + std::source_location::current().function_name() +
                                 " (" + std::source_location::current().file_name() + ")."
                               };
   }
   std::string token { };
   m_file >> token;
   if (!m_file)
   {
      throw std::runtime_error { std::string { } + "Error occured when reading name of SPL in file " +
                                 m_filename + " in " + std::source_location::current().function_name() +
                                 " (" + std::source_location::current().file_name() + ")."
                               };
   }
   m_spl = token;
   m_spl_specified = true;
}

void spl_specification_t::parse_features()
{
   if (m_feature_names_specified)
   {
      throw std::runtime_error { std::string { } + "Features specified more than once in file " + 
                                 m_filename + " in " + std::source_location::current().function_name() +
                                 " (" + std::source_location::current().file_name() + ")."
                               };
   }
   unsigned_integer f;
   m_file >> f;
   if (!m_file)
   {
      throw std::runtime_error { std::string { } + 
                                 "Error occured when reading number of independent features in file " +
                                 m_filename + " in " + std::source_location::current().function_name() +
                                 " (" + std::source_location::current().file_name() + ")."
                               };
   }
   if (f == 0u)
   {
      throw std::runtime_error { std::string { } + 
                                 "Number of independent features is specified as 0 in file " +
                                 m_filename + " in " + std::source_location::current().function_name() +
                                 " (" + std::source_location::current().file_name() + ")."
                               };
   }
   m_F = f;
   m_feature_names = feature_names_t(4); // Create as many elements as neccessary.
   for (auto& feature : m_feature_names)
   {
      std::string token;
      m_file >> token;
      if (!m_file)
      {
         throw std::runtime_error { std::string { } + 
                                    "Error occured when reading name of independent feature in file " +
                                    m_filename + " in " + std::source_location::current().function_name() +
                                    " (" + std::source_location::current().file_name() + ")."
                                  };
      }
      while (to_upper(token) == kw::COMMENT)
      {
         parse_skip_line();
         m_file >> token;
         if (!m_file)
         {
            throw std::runtime_error { std::string { } + 
                                       "Error occured when reading name of independent feature in file " +
                                       m_filename + " in " + std::source_location::current().function_name() +
                                       " (" + std::source_location::current().file_name() + ")."
                                     };
         }
      }
      feature = token;
   }
   m_feature_names_specified = true;
}

void spl_specification_t::parse_categories()
{
   if (m_categories_specified)
   {
      throw std::runtime_error { std::string { } + "Feature categories specified more than once in file " + 
                                 m_filename + " in " + std::source_location::current().function_name() +
                                 " (" + std::source_location::current().file_name() + ")."
                               };
   }
   std::string token;
   do
   {
      m_file >> token;
      if (!m_file)
      {
         throw std::runtime_error { std::string { } + 
                                    "Error occured when reading feature category in file " +
                                    m_filename + " in " + std::source_location::current().function_name() +
                                    " (" + std::source_location::current().file_name() + ")."
                                  };
      }
      token = to_upper(token);
      if (token == kw::F)
      {
         // Nothing to do, F is already (and always) included
      }
      else
      if (token == kw::A)
      {
         if (m_categories && A)
         {
            throw std::runtime_error { std::string { } + 
                                       "Multiple inclusion of feature category A in file " +
                                       m_filename + " in " + std::source_location::current().function_name() +
                                       " (" + std::source_location::current().file_name() + ")."
                                     };
        }
        m_categories |= A;
      }
      else 
      if (token == kw::O)
      {
         if (m_categories && O)
         {
            throw std::runtime_error { std::string { } + 
                                       "Multiple inclusion of feature category O in file " +
                                       m_filename + " in " + std::source_location::current().function_name() +
                                       " (" + std::source_location::current().file_name() + ")."
                                     };
        }
        m_categories |= O;
      }
      else 
      if (token == kw::N)
      {
         if (m_categories && N)
         {
            throw std::runtime_error { std::string { } + 
                                       "Multiple inclusion of feature category N in file " +
                                       m_filename + " in " + std::source_location::current().function_name() +
                                       " (" + std::source_location::current().file_name() + ")."
                                     };
        }
        m_categories |= N;
      }
      else 
      if (token == kw::AN)
      {
         if (m_categories && AN)
         {
            throw std::runtime_error { std::string { } + 
                                       "Multiple inclusion of feature category AN in file " +
                                       m_filename + " in " + std::source_location::current().function_name() +
                                       " (" + std::source_location::current().file_name() + ")."
                                     };
        }
        m_categories |= AN;
      }
      else 
      if (token == kw::ON)
      {
         if (m_categories && ON)
         {
            throw std::runtime_error { std::string { } + 
                                       "Multiple inclusion of feature category ON in file " +
                                       m_filename + " in " + std::source_location::current().function_name() +
                                       " (" + std::source_location::current().file_name() + ")."
                                     };
        }
        m_categories |= ON;
      }
      else 
      if (token == kw::ANF)
      {
         if (m_categories && ANF)
         {
            throw std::runtime_error { std::string { } + 
                                       "Multiple inclusion of feature category ANF in file " +
                                       m_filename + " in " + std::source_location::current().function_name() +
                                       " (" + std::source_location::current().file_name() + ")."
                                     };
        }
        m_categories |= ANF;
      }
      else 
      if (token == kw::ONF)
      {
         if (m_categories && ONF)
         {
            throw std::runtime_error { std::string { } + 
                                       "Multiple inclusion of feature category ONF in file " +
                                       m_filename + " in " + std::source_location::current().function_name() +
                                       " (" + std::source_location::current().file_name() + ")."
                                     };
        }
        m_categories |= ONF;
      }
      else
      if (token == kw::COMMENT)
      {
         parse_skip_line();
      }
      else
      if (token != kw::END)
      {
         throw std::runtime_error { std::string { } + 
                                    "Error occured when reading feature category in file " +
                                    m_filename + " in " + std::source_location::current().function_name() +
                                    " (" + std::source_location::current().file_name() + ")."
                                  };
      }
   }
   while (token != kw::END);
   m_categories_specified = true;
}

void spl_specification_t::parse_explicit_dependencies()
{
   if (m_explicit_dependencies_specified)
   {
      throw std::runtime_error { std::string { } + "Explicit dependencies specified more than once in file " + 
                                 m_filename + " in " + std::source_location::current().function_name() +
                                 " (" + std::source_location::current().file_name() + ")."
                               };
   }
   if (!m_feature_names_specified)
   {
      throw std::runtime_error { std::string { } + "Features must be specified before explicit dependencies in file " + 
                                 m_filename + " in " + std::source_location::current().function_name() +
                                 " (" + std::source_location::current().file_name() + ")."
                               };
   }
   std::string feature1,dependency,feature2,keyword;
   do
   {
      m_file >> feature1;
      if (!m_file)
      {
         throw std::runtime_error { std::string { } + 
                                    "Error occured when reading 1st feature of dependency in file " +
                                    m_filename + " in " + std::source_location::current().function_name() +
                                    " (" + std::source_location::current().file_name() + ")."
                                  };
      }
      keyword = to_upper(feature1);
      if (keyword == kw::COMMENT)
      {
         parse_skip_line();
      }
      else
      if (keyword != kw::END)
      {
         m_file >> dependency;
         if (!m_file)
         {
            throw std::runtime_error { std::string { } + 
                                       "Error occured when reading dependency in file " +
                                       m_filename + " in " + std::source_location::current().function_name() +
                                       " (" + std::source_location::current().file_name() + ")."
                                     };
         }
         dependency = to_upper(dependency);
         if (dependency != kw::REQUIRES || dependency != kw::EXCLUDES)
         {
            throw std::runtime_error { std::string { } + 
                                       "Dependency is specified as " + dependency + " in file " +
                                       m_filename + " in " + std::source_location::current().function_name() +
                                       " (" + std::source_location::current().file_name() + ")."
                                     };
         }
         m_file >> feature2;
         if (!m_file)
         {
            throw std::runtime_error { std::string { } + 
                                       "Error occured when reading 2nd feature of dependency in file " +
                                       m_filename + " in " + std::source_location::current().function_name() +
                                       " (" + std::source_location::current().file_name() + ")."
                                     };
         }
         parse_dependency(feature1,dependency,feature2);
      }
   } while (keyword != kw::END);
   m_explicit_dependencies_specified = true;
}

void spl_specification_t::parse_missing_systems()
{
   if (m_missing_systems_specified)
   {
      throw std::runtime_error { std::string { } + "Missing systems specified more than once in file " + 
                                 m_filename + " in " + std::source_location::current().function_name() +
                                 " (" + std::source_location::current().file_name() + ")."
                               };
   }
   if (m_available_systems_specified)
   {
      throw std::runtime_error { std::string { } + "Missing systems cannot be specified if " +
                                 "available systems have been specified before in file " + 
                                 m_filename + " in " + std::source_location::current().function_name() +
                                 " (" + std::source_location::current().file_name() + ")."
                               };
   }
   if (!m_feature_names_specified)
   {
      throw std::runtime_error { std::string { } + "Features must be specified before missing systems in file " + 
                                 m_filename + " in " + std::source_location::current().function_name() +
                                 " (" + std::source_location::current().file_name() + ")."
                               };
   }
   auto systems { power2(power2(F)) };
   std::string token;
   do
   {
      m_file >> token;
      if (!m_file)
      {
         throw std::runtime_error { std::string { } + 
                                    "Error occured when reading missing systems in file " +
                                    m_filename + " in " + std::source_location::current().function_name() +
                                    " (" + std::source_location::current().file_name() + ")."
                                  };
      }
      token = to_upper(token);
      if (token == kw::COMMENT)
      {
         parse_skip_line();
      }
      else
      if (token != kw::END)
      {
         unsigned_integer id { std::stoull(token) };
         if (id > systems)
         {
            throw std::runtime_error { std::string { } + 
                                       "Missing system with ID " + token + 
                                       "cannot exist in file " +
                                       m_filename + " in " + std::source_location::current().function_name() +
                                       " (" + std::source_location::current().file_name() + ")."
                                     };
         }
         if (m_missing_systems.contains(id))
         {
            throw std::runtime_error { std::string { } + 
                                       "Missing system with ID " + token + 
                                       " has been already specified in file " +
                                       m_filename + " in " + std::source_location::current().function_name() +
                                       " (" + std::source_location::current().file_name() + ")."
                                     };
         }
         m_missing_systems.insert(id);
      }
   } while (token != kw::END);
   m_missing_systems_specified = true;
}

void spl_specification_t::parse_available_systems()
{
   if (m_available_systems_specified)
   {
      throw std::runtime_error { std::string { } + "Available systems specified more than once in file " + 
                                 m_filename + " in " + std::source_location::current().function_name() +
                                 " (" + std::source_location::current().file_name() + ")."
                               };
   }
   if (m_missing_systems_specified)
   {
      throw std::runtime_error { std::string { } + "Available systems cannot be specified if " +
                                 "mising systems have been specified before in file " + 
                                 m_filename + " in " + std::source_location::current().function_name() +
                                 " (" + std::source_location::current().file_name() + ")."
                               };
   }
   if (!m_feature_names_specified)
   {
      throw std::runtime_error { std::string { } + "Features must be specified before missing systems in file " + 
                                 m_filename + " in " + std::source_location::current().function_name() +
                                 " (" + std::source_location::current().file_name() + ")."
                               };
   }
   auto systems { power2(power2(F)) };
   std::string token;
   do
   {
      m_file >> token;
      if (!m_file)
      {
         throw std::runtime_error { std::string { } + 
                                    "Error occured when reading available systems in file " +
                                    m_filename + " in " + std::source_location::current().function_name() +
                                    " (" + std::source_location::current().file_name() + ")."
                                  };
      }
      token = to_upper(token);
      if (token == kw::COMMENT)
      {
         parse_skip_line();
      }
      else
      if (token != kw::END)
      {
         unsigned_integer id { std::stoull(token) };
         if (id > systems)
         {
            throw std::runtime_error { std::string { } + 
                                       "Available system with ID " + token + 
                                       "cannot exist in file " +
                                       m_filename + " in " + std::source_location::current().function_name() +
                                       " (" + std::source_location::current().file_name() + ")."
                                     };
         }
         if (m_available_systems.contains(id))
         {
            throw std::runtime_error { std::string { } + 
                                       "Available system with ID " + token + 
                                       " has been already specified in file " +
                                       m_filename + " in " + std::source_location::current().function_name() +
                                       " (" + std::source_location::current().file_name() + ")."
                                     };
         }
         m_available_systems.insert(id);
      }
   } while (token != kw::END);
   m_available_systems_specified = true;
}

void spl_specification_t::parse_skip_line()
{
   m_file.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
}

void spl_specification_t::parse_dependency(const std::string& f1,const std::string& dependency, const std::string& f2)
{
   auto f1_index { feature_index(f1) },
        f2_index { feature_index(f2) };
   if (f1_index == zero)
   {
      throw std::runtime_error { std::string { } + 
                                 "Unknown feature " + f1 + " in dependency in file " +
                                 m_filename + " in " + std::source_location::current().function_name() +
                                 " (" + std::source_location::current().file_name() + ")."
                               };
   }
   if (f2_index == zero)
   {
      throw std::runtime_error { std::string { } + 
                                 "Unknown feature " + f2 + " in dependency in file " +
                                 m_filename + " in " + std::source_location::current().function_name() +
                                 " (" + std::source_location::current().file_name() + ")."
                               };
   }
   if (dependency == kw::REQUIRES)
   {
      m_requires.insert(std::make_pair(f1_index,f2_index));
      m_is_required_by.insert(std::make_pair(f2_index,f1_index));
   }
   if (dependency == kw::EXCLUDES)
   {
      m_excludes.insert(std::make_pair(f1_index,f2_index));
      m_is_excluded_by.insert(std::make_pair(f2_index,f1_index));
   }
}

unsigned_integer spl_specification_t::feature_index(const std::string& f)
{
   auto result { find(m_feature_names.begin(),m_feature_names.end(),f) };
   if (result == m_feature_names.end())
   {
      return zero;
   }
   return result - m_feature_names.begin() + one;
}
