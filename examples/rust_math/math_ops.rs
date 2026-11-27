pub fn sum(a: i32, b: i32) -> i32 {
    a + b
}

pub fn clamp_u32(value: u32, upper: u32) -> u32 {
    if value < upper { value } else { upper }
}

pub fn is_even(value: i32) -> bool {
    value % 2 == 0
}
