// xnet: xlibs's native companion, launched from the game over a Lua FFI CreateProcess. It does the
// out-of-sandbox work the LuaJIT sandbox forbids -- for now, one HTTP(S) request. Stdlib only, so
// `go build` yields a single static exe with zero runtime dependencies.
//
// It is generic: the caller gives it a method, a url, a header file, and a body file, and it knows
// nothing about what it carries. The auth token arrives in the header file, never on the command line,
// so it cannot leak through the process arguments.
package main

import (
	"bytes"
	"flag"
	"fmt"
	"io"
	"net/http"
	"os"
	"strings"
	"time"
)

func main() {
	method := flag.String("method", "POST", "HTTP method")
	url := flag.String("url", "", "request URL")
	headerFile := flag.String("header-file", "", "file of 'Key: Value' lines, one per header (holds the auth)")
	bodyFile := flag.String("body-file", "", "request body file")
	out := flag.String("out", "", "write the response body to this file")
	flag.Parse()

	if *url == "" {
		fmt.Fprintln(os.Stderr, "xnet: -url is required")
		os.Exit(2)
	}

	var body io.Reader
	if *bodyFile != "" {
		data, err := os.ReadFile(*bodyFile)
		if err != nil {
			fmt.Fprintln(os.Stderr, "xnet: body-file:", err)
			os.Exit(2)
		}
		body = bytes.NewReader(data)
	}

	req, err := http.NewRequest(strings.ToUpper(*method), *url, body)
	if err != nil {
		fmt.Fprintln(os.Stderr, "xnet: request:", err)
		os.Exit(2)
	}
	if *headerFile != "" {
		data, err := os.ReadFile(*headerFile)
		if err != nil {
			fmt.Fprintln(os.Stderr, "xnet: header-file:", err)
			os.Exit(2)
		}
		for _, line := range strings.Split(string(data), "\n") {
			line = strings.TrimRight(line, "\r")
			if i := strings.Index(line, ":"); i > 0 {
				req.Header.Set(strings.TrimSpace(line[:i]), strings.TrimSpace(line[i+1:]))
			}
		}
	}

	resp, err := (&http.Client{Timeout: 30 * time.Second}).Do(req)
	if err != nil {
		fmt.Fprintln(os.Stderr, "xnet: send:", err)
		os.Exit(1)
	}
	defer resp.Body.Close()
	respBody, _ := io.ReadAll(resp.Body)
	if *out != "" {
		if err := os.WriteFile(*out, respBody, 0o644); err != nil {
			fmt.Fprintln(os.Stderr, "xnet: out:", err)
			os.Exit(1)
		}
	}
	fmt.Println(resp.StatusCode)
	if resp.StatusCode < 200 || resp.StatusCode >= 300 {
		os.Exit(1)
	}
}
