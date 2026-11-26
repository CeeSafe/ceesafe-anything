def clamp_int(x: int, lo: int, hi: int) -> int:
    if x < lo:
        return lo
    if x > hi:
        return hi
    return x


def weighted_score(base: int, bonus: int) -> int:
    return base * 2 + bonus
