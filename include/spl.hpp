namespace kw
{
   constexpr ::std::string_view
      COMMENT { "//" },
      SPL { "SPL" },
      FEATURES { "FEATURES" },
      CATEGORIES { "CATEGORIES" },
      END { "END" },
      EXPLICIT_DEPENDENCIES { "EXPLICIT_DEPENDENCIES" },
      REQUIRES { "REQUIRES" },
      EXCLUDES { "EXCLUDES" },
      MISSING_SYSTEMS { "MISSING_SYSTEMS" },
      AVAILABLE_SYSTEMS { "AVAILABLE_SYSTEMS" };
}

//using namespace std::literals::string_literals;

class spl_specification_t final
{
   public:
      explicit spl_specification_t(const std::string& filename);
      ~spl_specification_t();
      [[nodiscard]] bool setup();
      const ::std::string m_filename;
      ::std::ofstream m_file;
      const bool m_success;
};

spl_specification_t::spl_specification_t(const std::string& filename):
   m_filename { filename },m_success { setup() }
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
      throw ::std::runtime_error { std::string { } + "Cannot open file named " +
                                 m_filename + " in " + 
                                 ::std::source_location::current().function_name() +
                                 " (" + 
                                 ::std:: source_location::current().file_name() + 
                                 ")."
                               };

   }
   return true;
}
