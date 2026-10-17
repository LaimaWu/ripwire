package packager

import (
	"errors"
	"fmt"
	"io"
	"path/filepath"
	"strings"
)

var errInsufficientParams = errors.New("a target file or a packager name is required")

// Packager turns a file list into one archive format.
type Packager struct {
	Format string
	Files  []string
}

// Package writes the archive for info: it picks the format, then writes every file into the archive.
func (p *Packager) Package(target string, w io.Writer) error {
	if target == "" && p.Format == "" {
		return errInsufficientParams
	}
	if p.Format == "" {
		fmt.Println("guessing packager from target file extension...")
		p.Format = strings.TrimPrefix(filepath.Ext(target), ".")
	}
	return writeArchive(p.Format, p.Files, w)
}

func writeArchive(format string, files []string, w io.Writer) error {
	for _, f := range files {
		if _, err := fmt.Fprintf(w, "%s:%s\n", format, f); err != nil {
			return err
		}
	}
	return nil
}
