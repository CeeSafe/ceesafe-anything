export function add_numbers(a, b) {
  return a + b;
}

export function clamp_value(x, lo, hi) {
  if (x < lo) return lo;
  if (x > hi) return hi;
  return x;
}
