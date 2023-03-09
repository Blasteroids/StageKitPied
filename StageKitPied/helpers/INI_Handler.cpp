#include "INI_Handler.h"

INI_Handler::INI_Handler() {
  // A few defaults.
  m_file_buffer_size = 0;
  m_section_start    = '[';
  m_section_end      = ']';
  m_deliminator      = '=';
  m_comment          = '#';
  m_section_current  = NULL;
  m_has_file_loaded  = false;
}

INI_Handler::~INI_Handler() {
  this->ClearAll();
}

void INI_Handler::ClearAll() {
  // No longer has data and therefore is no longer loaded.
  m_has_file_loaded = false;

  // Clean up sections and tokens mem.
  this->ClearSectionsAndTokens();

  // Incase we never released the buffer mem.
  this->ClearBuffer();
}

void INI_Handler::SetSectionStart( const char& section_start_char ) {
  m_section_start = section_start_char;
}

void INI_Handler::SetSectionEnd( const char& section_end_char ) {
  m_section_end = section_end_char;
}

void INI_Handler::SetComment( const char& comment_char ) {
  m_comment = comment_char;
}

void INI_Handler::SetDeliminator( const char& deliminator_char ) {
  m_deliminator = deliminator_char;
}

bool INI_Handler::Load( const std::string& filename ) {

  MSG_INI_DEBUG( "file = '" << filename << "'" );


  // Since everything gets cleared before a load we clear this little toggle.
  m_has_file_loaded = false;

  // Clean up sections and tokens mem.
  this->ClearSectionsAndTokens();

  // Incase we never released the buffer mem.
  this->ClearBuffer();

  // Fill internal buffer.
  if( !this->FillBuffer( filename ) ) {
    m_error_msg = "INI_Handler: Error filling buffer.";
    return false;
  }

  // Generate a "default" section, which is our current section.
  m_section_current = new INI_Section;
  m_section_list.push_back( m_section_current );


  MSG_INI_DEBUG( "About to process buffer." );

  // Process internal buffer.
  m_has_file_loaded = this->ProcessBuffer();

  if( !m_has_file_loaded ) {
    m_error_msg = "INI_Handler: Error process buffer.";
  }

  // Clean up.
  this->ClearBuffer();

  // Return the result from the buffer process.
  return m_has_file_loaded;
}

bool INI_Handler::HasFileLoaded() {
  return m_has_file_loaded;
}

bool INI_Handler::CreateSection( const std::string& section_name ) {
  // Creates a new section with the given name.
  // If the section is already existing, then we just change to
  // that one.
  if( this->SetSection( section_name ) ) {
    return false;
  }

  m_section_current = new INI_Section;
  m_section_current->m_name = section_name;
  m_section_list.push_back( m_section_current );

  return true;
}

bool INI_Handler::DeleteSection( const std::string& section_name ) {
  // If it is not a section, then it is removed already.
  std::vector< INI_Section* >::iterator itr    = m_section_list.begin();
  std::vector< INI_Section* >::iterator itrEnd = m_section_list.end();
  for( ; itr != itrEnd; itr++ ) {
    if( (*itr)->m_name == section_name ) {
      if( m_section_current != NULL ) {
        if( m_section_current->m_name == section_name ) {
          m_section_current = NULL;
        }
      }

      // free the memory
      delete (*itr);

      // Remove pointer from vector.
      m_section_list.erase( itr );

      return true;
    }
  }

  return false;
}

bool INI_Handler::SetToken( const std::string& token_name, const std::string& token_value ) {
  // Ensure we are already in a section.
  if( m_section_current == NULL ) {
    return false;
  }

  // First check to see if token is already existing.
  INI_Token* ptr_token = this->GetToken( token_name );

  if( ptr_token == NULL ) {
    // Create token.
    ptr_token               = new INI_Token;
    ptr_token->m_name       = token_name;
    ptr_token->m_linenumber = 0;  // No real need for a line number on create.
    ptr_token->m_value      = token_value;
    ptr_token->m_read       = false;
    m_section_current->m_tokenlist.push_back( ptr_token );

  } else {
    // Just update token values.
    ptr_token->m_value = token_value;
    ptr_token->m_read  = false;
  }

  return true;
}

std::string INI_Handler::GetErrorMessage() {
  return m_error_msg;
}

bool INI_Handler::FindSection( const std::string& section_name ) {
  std::vector< INI_Section* >::iterator itr    = m_section_list.begin();
  std::vector< INI_Section* >::iterator itrEnd = m_section_list.end();
  for( ; itr != itrEnd; itr++ ) {
    if( (*itr)->m_name == section_name ) {
      return true;
    }
  }
  return false;
}

bool INI_Handler::SetSection( uint32_t section_number ) {
  if( section_number > m_section_list.size() || section_number == 0 ) {
    return false;
  }

  --section_number;

  m_section_current = m_section_list[ section_number ];

  return true;
}

bool INI_Handler::SetSection( const std::string& section_name ) {
  if( m_section_current != NULL ) {
    // Avoid hunting around if we are already in it.
    if( m_section_current->m_name == section_name ) {
      return true;
    }
  }

  std::vector< INI_Section* >::iterator itr    = m_section_list.begin();
  std::vector< INI_Section* >::iterator itrEnd = m_section_list.end();
  for( ; itr != itrEnd; itr++ ) {
    if( (*itr)->m_name == section_name ) {
      m_section_current = (*itr);
      return true;
    }
  }
  return false;
}

std::string INI_Handler::GetSection() {
  return m_section_current->m_name;
}

int INI_Handler::GetTokenValue( const std::string& token_name ) {
  INI_Token* ptrTkn = this->GetToken( token_name );

  if( ptrTkn == NULL ) {
    m_error_msg  = "Unable to find token '";
    m_error_msg += token_name;
    m_error_msg += "' in section ";
    m_error_msg += m_section_current->m_name;

    return 0;
  }

  ptrTkn->m_read = true;

  return stoi( ptrTkn->m_value );
};

bool INI_Handler::GetTokenBool( const std::string& token_name ) {
  INI_Token* ptrTkn = this->GetToken( token_name );

  if( ptrTkn == NULL ) {
    m_error_msg  = "Unable to find token '";
    m_error_msg += token_name;
    m_error_msg += "' in section ";
    m_error_msg += m_section_current->m_name;

    return false;
  }

  ptrTkn->m_read = true;

  return stoi( ptrTkn->m_value ) > 0 ? true : false;
};

std::string INI_Handler::GetTokenString( const std::string& token_name ) {
  INI_Token* ptrTkn = this->GetToken( token_name );

  if( ptrTkn == NULL ) {
    m_error_msg  = "Unable to find token '";
    m_error_msg += token_name;
    m_error_msg += "' in section ";
    m_error_msg += m_section_current->m_name;

    return std::string();
  }

  ptrTkn->m_read = true;

  return ptrTkn->m_value;
};

bool INI_Handler::TokenExists( const std::string& token_name ) {
  INI_Token* ptrTkn = this->GetToken( token_name );

  return ( ptrTkn != NULL );
};

uint32_t INI_Handler::GetNumberOfSections() {
  return m_section_list.size();
};

int32_t INI_Handler::GetTokenLine( const std::string& token_name ) {
  INI_Token* ptrTkn = this->GetToken( token_name );

  return ( ptrTkn == NULL ? (int32_t)-1 : (int32_t)ptrTkn->m_linenumber );
}

INI_Token* INI_Handler::GetToken( const std::string& token_name ) {
  std::vector< INI_Token* >::iterator itr    = m_section_current->m_tokenlist.begin();
  std::vector< INI_Token* >::iterator itrEnd = m_section_current->m_tokenlist.end();
  for( ; itr != itrEnd; itr++ ) {
    if( (*itr)->m_name == token_name ) {
      return (*itr);
    }
  }
  return NULL;
}

bool INI_Handler::ProcessBuffer() {
  char* ptr_pos   = m_ptr_file_buffer;
  char* ptr_end   = m_ptr_file_buffer + m_file_buffer_size;
  char* ptr_start = m_ptr_file_buffer;

  uint32_t lineNumber = 0;

  // Loop through until end of buffer.
  while( ptr_pos < ptr_end ) {
    // New line, so we might be able to process it.
    if( *ptr_pos == 10 ) {
      // Remove whitespace at start
      for( ; *ptr_start == ' '; ptr_start++ );

      // Ensure its not just a blank line.
      if( ptr_start < ptr_pos ) {
        MSG_INI_DEBUG( "About to ProcessLine." );
        if( !this->ProcessLine( ptr_start, ptr_pos, lineNumber ) ) {
          return false;
        }
      }

      // Set the start of next line onto the next part of the buffer
      // Skipping the '\n'
      ptr_start = ptr_pos + 1;
      lineNumber++;
    }

    // Move out position pointer onwards.
    ++ptr_pos;
  }

  // Anything left at end of file?
  if( ptr_start < ptr_end ) {
    // Attempt to process it.  Might not be a '\n' at eof.
    if( !this->ProcessLine( ptr_start, ptr_pos, lineNumber ) ) {
      return false;
    }
  }

  return true;
}

bool INI_Handler::ProcessLine( char* ptr_line, char* ptr_line_end, uint32_t line_number ) {
  // Ignore comments
  if( *ptr_line == m_comment ) {
    return true;
  }

  // Is it a section start ?
  if( *ptr_line == m_section_start ) {
    ++ptr_line;

    // Try to find the section finish.
    char* ptr_section_start = ptr_line;
    while( ptr_line < ptr_line_end && *ptr_line != m_section_end ) {
      ++ptr_line;
    }

    // If we found a section finish, then we got a section name!
    if( *ptr_line == m_section_end ) {
      std::string section_name( ptr_section_start, ptr_line - ptr_section_start );

      // If section does not exist, then create a new section.
      if( !this->SetSection( section_name ) ) {
        MSG_INI_DEBUG( "Adding new section. '" << section_name << "'" );

        m_section_current = new INI_Section;
        m_section_current->m_name = section_name;
        m_section_list.push_back( m_section_current );
      }

      return true;
    }

    m_error_msg  = "Malformed Section Detected At Line ";
    m_error_msg += std::to_string( line_number );
    return false;
  }

  // Otherwise we are dealing with token and value.

  char* ptr_value = ptr_line;
  while( ptr_value < ptr_line_end && *ptr_value != m_deliminator ) {
    ++ptr_value;
  }

  // Skip past deliminator
  ++ptr_value;

  // Check for malformed line.
  if( ptr_value > ptr_line_end ) {
    m_error_msg  = "Malformed Line Detected At ";
    m_error_msg += std::to_string( line_number );
    return false;
  }

  std::string token_name( ptr_line, ( ptr_value - ptr_line - 1 ) );

  MSG_INI_DEBUG( "Testing for existing token '" << token_name << "'" );

  int32_t token_line = this->GetTokenLine( token_name );

  if( token_line != -1 ) {
    m_error_msg  = "Duplicate token found at line ";
    m_error_msg += std::to_string( line_number );
    m_error_msg += " previously declared at line ";
    m_error_msg += std::to_string( token_line );
    m_error_msg += " for section '";
    m_error_msg += m_section_current->m_name;
    m_error_msg += "'";
    return false;
  }

  INI_Token* ptr_token = new INI_Token;
  ptr_token->m_name = token_name;
  ptr_token->m_linenumber = line_number;

  // We can have blank values.
  if( ptr_value != ptr_line_end ) {
    ptr_token->m_value.assign( ptr_value, ptr_line_end - ptr_value );
  }
  ptr_token->m_read = false;

  MSG_INI_DEBUG( "Adding new token '" << token_name << "' from line " << line_number << " with value '" << ptr_token->m_value << "'");

  m_section_current->m_tokenlist.push_back( ptr_token );

  return true;
}

void INI_Handler::ClearBuffer() {
  // Ensure buffer is empty.
  if( m_file_buffer_size > 0 ) {
    delete [] m_ptr_file_buffer;
    m_file_buffer_size = 0;
  }
}

void INI_Handler::ClearSectionsAndTokens() {
  // Delete all sections and tokens
  std::vector< INI_Section* >::iterator section_iterator_end = m_section_list.end();
  std::vector< INI_Token* >::iterator token_iterator;
  std::vector< INI_Token* >::iterator token_iterator_end;

  for( std::vector< INI_Section* >::iterator section_iterator = m_section_list.begin();
       section_iterator != section_iterator_end;
       section_iterator++ ) {

    if( (*section_iterator)->m_tokenlist.size() != 0 ) {
      token_iterator_end = (*section_iterator)->m_tokenlist.end();
      for( token_iterator = (*section_iterator)->m_tokenlist.begin();
           token_iterator != token_iterator_end;
           token_iterator++ ) {
        delete (*token_iterator);
      }
    }
    delete (*section_iterator);
  }
  m_section_list.clear();
}

bool INI_Handler::FillBuffer( const std::string& filename ) {
  // Open the file.
  std::ifstream file_input;

  // Open at end of file to be able to use tellg for filesize
  file_input.open( filename, std::ios::in | std::ios::binary | std::ios::ate );

  if( !file_input.is_open() ) {
    m_error_msg = "Unable to open file";
    return false;
  }

  // Read the size of the file and ensure its got something.
  m_file_buffer_size = file_input.tellg();

  if( m_file_buffer_size == 0 ) {
    m_error_msg = "File size reported as 0 bytes.";
    return false;
  }

  // Temporary buffer to load the file into.
  char* ptr_tmp_buffer = new char[ m_file_buffer_size ];

  // Move to start of file, read file data into temp buffer & then close file.
  file_input.seekg( 0, std::ios::beg );
  file_input.read( ptr_tmp_buffer, m_file_buffer_size );
  file_input.close();

  // Now we check for file read error.  Bytes In should equal file size.
  uint32_t bytes_read = file_input.gcount();

  if( bytes_read != m_file_buffer_size ) {
    // File size mismatch from read.
    m_error_msg = "Error during file read.  Read size not same as file size!";
    delete [] ptr_tmp_buffer;
    return false;
  }

  // Generate enough room to store whole file.
  m_ptr_file_buffer = new char[ m_file_buffer_size ];
  m_file_buffer_size = 0;

  // Strip out any unwanted chars
  char* destination = m_ptr_file_buffer;
  char* source      = ptr_tmp_buffer;

  while( bytes_read-- ) {
    if( *source == 10 || *source > 31 ) {
      *destination++ = *source;
      m_file_buffer_size++;
    }
    source++;
  }

  // Clean up the temporary storage.
  delete [] ptr_tmp_buffer;

  // This could help ;)
  return true;
}
