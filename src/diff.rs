pub fn discover_mismatch(fsfiles: Vec<String>, dbfiles: Vec<String>) -> Vec<String> {
    let mut fs_db_mismatch: Vec<String> = Vec::new();
    for fsfile in &fsfiles {
        for dbfile in &dbfiles {
            if dbfile != fsfile {
                fs_db_mismatch.push(fsfile.clone());
            }
        }
    }

    fs_db_mismatch

}