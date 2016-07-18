namespace cpp succinct

service Server {
  // Initialize
  i32 Initialize(),
	
  // Supported operations
  void Append(1: i64 key, 2: string value),
  string Get(1: i64 key),
  list<i64> Search(1: string query),
  void Delete(1: i64 key),

  // Metadata
  i64 GetNumKeys(),
  i64 GetSize(),
}
