#ifndef INI_HANDLER_H_
#define INI_HANDLER_H_

#ifdef DEBUG
  #define MSG_INI_DEBUG( str ) do { std::cout << "INI_Handler : DEBUG : " << str << std::endl; } while( false )
#else
  #define MSG_INI_DEBUG( str ) do { } while ( false )
#endif

#define MSG_INI_ERROR( str ) do { std::cout << "INI_Handler : ERROR : " << str << std::endl; } while( false )
#define MSG_INI_INFO( str ) do { std::cout << "INI_Handler : INFO : " << str << std::endl; } while( false )

#include <cstdint>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <memory>

// The basic token struct
struct INI_Token
{
  // The name of this token.
  std::string m_name;

  // The value this token has, stored in string format.
  std::string m_value;

  // The line number this token was read from.
  uint32_t m_linenumber;

  // If this token has been 'acquired'
  bool m_read;
};

// The basic section.
struct INI_Section
{
  // The name of this section.
  std::string m_name;

  // A vector containing the tokens of this section.
  std::vector< INI_Token* > m_tokenlist;
};

// Quick and dirrty ini handler.
class INI_Handler
{
public:

  // Constructor
  /* The following default ini settings are applied on class creation.
   * SectionStart = '['
   * SectionEnd   = ']'
   * Deliminator  = '='
   * Comment      = '#'
   */
  INI_Handler();

  // Destructor
  ~INI_Handler();

  // One shot clear mem
  /* Clears all sections, items, tokens and any held mem.
   */
  void ClearAll();

  // Sets the section start character.
  /*
   * \param sectionStart  A character which marks the start of a section.
   */
  void SetSectionStart( const char& section_start_char );

  // Sets the section end character.
  /*
   * \param sectionEnd  A character which marks the end of a section.
   */
  void SetSectionEnd( const char& section_end_char );

  // Sets the comment character.
  /* Lines which start with this character are ignored.
   *
   * \param comment  A character which marks comments.
   */
  void SetComment( const char& comment_char );

  // Sets the deliminator character.
  /* This character is the item between a token and its value.
   *  i.e. name=foo  The deliminator is '='
   *
   * \param deliminator  A character which marks the deliminator between token and value.
   */
  void SetDeliminator( const char& deliminator_char );

  // Loads an ini/settings file.
  /*
   * \param filename  The filename to load.
   *
   * \return false on error, else true.
   */
  bool Load( const std::string& filename );

  // Indicates if this handler has a file loaded.
  /*
   * \return true if data has been loaded, else false.
   */
  bool HasFileLoaded();

  // Creates a new section.
  /* Will also set the current section to the given name.
   *
   * \param  sectionName  The section name.
   *
   * \return  false if section already created, else true.
   */
  bool CreateSection( const std::string& section_name );

  // Deletes a section.
  /* If mCurrentSection is the one to delete, then it becomes NULL.
   *
   * \param  sectionName  The section name to delete.
   *
   * \return  true on deletion, else false.
   */
  bool DeleteSection( const std::string& section_name );

  // Sets the value of the token in the given section.
  /* If the token is not found, then it creates the token.
   *
   * \param  tokenName   The token name to set.
   * \param  tokenValue  The value of the token.
   *
   * \return  false if not already in a section, else true.
   */
  bool SetToken( const std::string& token_name, const std::string& token_value );

  // Gets the last generated error.
  /*
   * \return std::string error message.
   */
  std::string GetErrorMessage();

  // Finds a section in the loaded ini file.
  /* Unlike SetSection, this does NOT set the current section.
   *
   * \param sectionName  The section name to find.
   *
   * \return true if the section name was found, else false.
   */
  bool FindSection( const std::string& section_name );

  // Sets the current section to a section in the loaded ini file.
  /* Note: Section numbers start from 1, and are as laid out in ini file.
   *
   * \param sectionNumber  The section number.
   *
   * \return true if the section name was set, else false.
   */
  bool SetSection( uint32_t section_number );

  // Sets the current section to a section in the loaded ini file.
  /*
   * \param sectionName  The section name to be the current section.
   *
   * \return true if the section name was found and set, else false.
   */
  bool SetSection( const std::string& section_name );

  // Gets the current section name.
  /*
   * \return pointer to the section name, or DT_NULL if no current section set.
   */
  std::string GetSection();

  // Gets the token value in the current section.
  /* Note: If the tokenname is not found, then 0 is returned.
   *
   * \param tokenName  The name of the token to find.
   *
   * \return The value of the token.
   */
  int GetTokenValue( const std::string& token_name );

  // Gets the token value as a boolean in the current section.
  /* Note: If the tokenname is not found, then false is returned.
   *
   * \param tokenName  The name of the token to find.
   *
   * \return If value > 0 then true else false.
   */
  bool GetTokenBool( const std::string& token_name );

  // Gets the token value as a char* in the current section.
  /* Note: If the tokenname is not found, then NULL is returned.
   *
   * \param tokenName  The name of the token to find.
   *
   * \return String
   */
  std::string GetTokenString( const std::string& token_name );

  // Gets the amount of sections.
  /*
   * \return  The number of sections.
   */
  uint32_t GetNumberOfSections();

  // Finds out if a token exists in the current section.
  /*
   * \param tokenName  The name of the token to find.
   *
   * \return true if token exists, else false.
   */
  bool TokenExists( const std::string& token_name );

  // Dumps unused items.
  /* For debugging only.
   *
   */
  void DumpUnused()
  {
    std::vector< INI_Section* >::iterator section_iterator;
    std::vector< INI_Token* >::iterator token_iterator;
    for( section_iterator = m_section_list.begin();
         section_iterator != m_section_list.end();
         section_iterator++ ) {
      if( (*section_iterator)->m_tokenlist.size() > 0 ) {
        MSG_INI_INFO( " Section - " << (*section_iterator)->m_name << " -" );
        for( token_iterator = (*section_iterator)->m_tokenlist.begin();
             token_iterator != (*section_iterator)->m_tokenlist.end();
             token_iterator++ ) {
          if( !(*token_iterator)->m_read ) {
            MSG_INI_INFO( "  Token - " << (*token_iterator)->m_name << " : " << (*token_iterator)->m_value );
          }
        }
      }
    }

  };

private:

  // Gets the file line number of the token in the current section.
  /*
   * \param tokenName  The token name to find.
   *
   * \return -1 if token name does not exist in current section, else its the file line number.
   */
  int32_t GetTokenLine( const std::string& token_name );

  // Gets the token in the current section.
  /*
   * \param tokenName  The token name to find.
   *
   * \return NULL if not found, else a pointer to the token.
   */
  INI_Token* GetToken( const std::string& token_name );

  // Processes the loaded in file buffer.
  /* Pulls out lines from the buffer and sends them to ProcessLine
   *
   * \return false on error, else true.
   */
  bool ProcessBuffer();

  // Processes a line into section or token records.
  /*
   * \param ptrLine     A pointer to the start of a line to process.
   * \param ptrLineEnd  A pointer to where the line ends.
   * \param lineNumber  The number of the line in the file.
   *
   * \return false on error, else true.
   */
  bool ProcessLine( char* ptr_line, char* ptr_line_end, uint32_t line_number );

  // Releases the memory of the file buffer.
  void ClearBuffer();

  // Releases the memory of the section and token data.
  void ClearSectionsAndTokens();

  // Fills the file buffer with the given file.
  /*
   * \param filename  The file to try and load into memory.
   *
   * \return false on error, else true.
   */
  bool FillBuffer( const std::string& filename );

  // The internal file buffer
  char*    m_ptr_file_buffer;

  // The size of the internal buffer used.
  uint32_t m_file_buffer_size;

  // The start section character.
  char m_section_start;

  // The end section character.
  char m_section_end;

  // The comment character.
  char m_comment;

  // The deliminator character.
  char m_deliminator;

  // Storage for sections.
  std::vector< INI_Section* > m_section_list;

  // The current section in use.
  INI_Section* m_section_current;

  // For reporting errors.
  std::string m_error_msg;

  // Indicates if this handler has a file loaded.
  bool m_has_file_loaded;
};


#endif

