package model

type Config struct{ Name string }

func Make() *Config { return &Config{Name: "x"} }

func Build() int { return 3 }

func Count() int { return 4 }

func Group() int { return 5 }
