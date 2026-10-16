//! Indexer: walks a crate's module tree and records every item it finds.

use std::collections::HashMap;

pub struct Item {
    pub name: String,
    pub kind: &'static str,
    pub line: u32,
}

pub struct Indexer {
    items: HashMap<String, Item>,
    skipped: u32,
}

impl Indexer {
    pub fn new() -> Self {
        Indexer { items: HashMap::new(), skipped: 0 }
    }

    /// Index a crate: walk its modules, keep the public items, count what was skipped.
    pub fn index(&mut self, root: &str, sources: &[(&str, &str)]) -> usize {
        let order = walk(root, sources);
        for path in order {
            let text = sources.iter().find(|(p, _)| *p == path).map(|(_, t)| *t).unwrap_or("");
            for (n, line) in text.lines().enumerate() {
                if let Some(rest) = line.trim().strip_prefix("pub fn ") {
                    let name = rest.split('(').next().unwrap_or("").to_string();
                    self.items.insert(name.clone(), Item { name, kind: "fn", line: n as u32 + 1 });
                } else if line.trim().starts_with("fn ") {
                    self.skipped += 1; // private items are not part of the crate surface
                }
            }
        }
        if self.items.is_empty() {
            eprintln!("indexer: crate {} has no public surface", root);
        }
        self.items.len()
    }
}

/// Walk the module tree breadth-first from the crate root and return the source paths in visit order.
pub fn walk(root: &str, sources: &[(&str, &str)]) -> Vec<String> {
    let mut out = vec![root.to_string()];
    let mut i = 0;
    while i < out.len() {
        let here = out[i].clone();
        let text = sources.iter().find(|(p, _)| *p == here).map(|(_, t)| *t).unwrap_or("");
        for line in text.lines() {
            if let Some(m) = line.trim().strip_prefix("mod ") {
                out.push(format!("{}/{}.rs", here.trim_end_matches(".rs"), m.trim_end_matches(';')));
            }
        }
        i += 1;
    }
    out
}
