package term

// A PACKAGE-level namesake of the builtin max: arm (G)'s `C external` census row for algo.Collect's max( … ) needs an
// in-repo symbol of that name. The function-local `var max` below used to be that symbol; since train 30 (go-local-var)
// a function-local var is not indexed, so it no longer is one, and this declaration keeps the arm's premise.
var max = 1.0

// Scale holds a local variable spelled like the builtin max.
func Scale(xs []float64) float64 {
	var max float64
	for _, x := range xs {
		if x > max {
			max = x
		}
	}
	return max
}
