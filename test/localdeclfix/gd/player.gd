extends Node

const SCRIPT_MAX := 5

var script_health = 10

class Inner:
	const INNER_MAX := 2
	var inner_hp = 1

func take_damage(amount):
	const LOCAL_MAX := 3
	var local_hp = script_health - amount
	return min(local_hp, LOCAL_MAX) + SCRIPT_MAX
