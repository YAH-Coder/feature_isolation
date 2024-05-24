class spl_specification_t
{
   public:
      explicit spl_specification(const std::string& filename);
      const std::string m_filename;
};

spl_specification_t::spl_specification(const std::string& filename):
   m_filename { filename }
{
}
