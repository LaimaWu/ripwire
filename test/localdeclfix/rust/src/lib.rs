pub const MODULE_LIMIT: usize = 8;
pub static MODULE_NAME: &str = "m";

pub mod inner {
    pub const INNER_LIMIT: usize = 2;
    pub static INNER_NAME: &str = "i";
}

pub struct Holder;

impl Holder {
    pub const ASSOC_LIMIT: usize = 4;

    pub fn method_local() -> usize {
        const METHOD_LOCAL: usize = 6;
        METHOD_LOCAL
    }
}

pub trait Shape {
    const TRAIT_SIDES: usize;
}

impl Shape for Holder {
    const TRAIT_SIDES: usize = 3;
}

extern "C" {
    static EXTERN_ERRNO: i32;
}

pub fn fn_local() -> usize {
    const FN_LOCAL_LIMIT: usize = 3;
    static FN_LOCAL_NAME: &str = "f";
    let closure = || {
        const CLOSURE_LOCAL: usize = 1;
        CLOSURE_LOCAL
    };
    FN_LOCAL_LIMIT + FN_LOCAL_NAME.len() + closure() + MODULE_LIMIT
}
