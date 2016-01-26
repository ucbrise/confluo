namespace cpp succinct

service Server {
	// Initialize
	i32 Initialize(),
	
	// Supported operations
	i32 Append(1: i64 key, 2: string value),
	string Get(1: i64 key),
	set<i64> Search(1: string query),
	i64 Dump(1: string path),
	i64 Load(1: string path),

  // Metadata
  i64 GetNumKeys(),
  i64 GetSize(),
}
