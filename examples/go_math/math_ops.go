package mathops

func AddI32(a int32, b int32) int32 {
	return a + b
}

func ClampI32(x int32, lo int32, hi int32) int32 {
	if x < lo {
		return lo
	}
	if x > hi {
		return hi
	}
	return x
}
