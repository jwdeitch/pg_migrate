extern crate walkdir;

pub fn walk(dir: String) {
    for entry in walkdir::WalkDir::new(dir) {
        let entry = entry.unwrap();

        if should_skip(&entry) {
            continue;
        }


        println!("{}", entry.path().display());
    }

}

fn should_skip(entry: &walkdir::DirEntry) -> bool {
    entry.file_name()
        .to_str()
        .map(|s| s.starts_with(".") || s.ends_with(".sql"))
        .unwrap_or(false)
}

