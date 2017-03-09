extern crate walkdir;

pub fn walk(dir: String, is_up: bool) -> Vec<String> {
    let mut all_migration_files: Vec<String> = Vec::new();
    for entry in walkdir::WalkDir::new(dir).follow_links(true) {
        let entry = entry.unwrap();

        if process_file(&entry, &is_up) {
            continue;
        }
//        println!("{}", entry.path().display());
        all_migration_files.push(entry.file_name().to_str().unwrap().to_string());
    }

    all_migration_files
}

fn process_file(entry: &walkdir::DirEntry, is_up: &bool) -> bool {
    entry.file_name()
        .to_str()
        .map(|s| {
            if *is_up && s.ends_with("-up.sql") {
                false
            } else if !*is_up && s.ends_with("-down.sql") {
                false
            } else {
                true
            }
        })
        .unwrap_or(false)
}