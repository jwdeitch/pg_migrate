extern crate walkdir;

pub fn walk(dir: String, is_up: bool) {
    for entry in walkdir::WalkDir::new(dir) {
        let entry = entry.unwrap();

        if should_skip(&entry, &is_up) {
            continue;
        }

        println!("{}", entry.path().display());
    }
}

fn should_skip(entry: &walkdir::DirEntry, is_up: &bool) -> bool {
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