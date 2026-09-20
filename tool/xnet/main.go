// xnet: xlibs's native companion, launched from the game over a Lua FFI CreateProcess. It does the
// out-of-sandbox work the LuaJIT sandbox forbids. Stdlib only, so `go build` yields a single static
// exe with zero runtime dependencies.
//
// Two modes:
//   xnet -url U [-method M] [-header-file H] [-body-file B] [-out O]   generic HTTP(S) request
//   xnet github-put -repo R -path P -file F -token-file T [-branch] [-message]   commit a file to a repo
//
// The auth token always arrives from a file, never on the command line, so it cannot leak through the
// process arguments.
package main

import (
	"bytes"
	"encoding/base64"
	"encoding/json"
	"flag"
	"fmt"
	"io"
	"net/http"
	"os"
	"strings"
	"time"
)

const apiBase = "https://api.github.com"

var client = &http.Client{Timeout: 30 * time.Second}

func main() {
	if len(os.Args) > 1 && os.Args[1] == "github-put" {
		githubPut(os.Args[2:])
		return
	}
	genericRequest()
}

// genericRequest: one HTTP(S) call the caller fully composes.
func genericRequest() {
	method := flag.String("method", "POST", "HTTP method")
	url := flag.String("url", "", "request URL")
	headerFile := flag.String("header-file", "", "file of 'Key: Value' header lines (holds the auth)")
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
		applyHeaderFile(req, *headerFile)
	}
	resp, err := client.Do(req)
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

// githubPut: commit one file to a repo through the GitHub Contents API, creating or updating it. The
// caller supplies the content; xnet base64-encodes it, reads the current sha for an update, and PUTs.
func githubPut(argv []string) {
	fs := flag.NewFlagSet("github-put", flag.ExitOnError)
	repo := fs.String("repo", "", "owner/name")
	path := fs.String("path", "", "file path within the repo")
	file := fs.String("file", "", "local file whose content to commit")
	tokenFile := fs.String("token-file", "", "file holding the PAT")
	branch := fs.String("branch", "", "target branch (default the repo default)")
	message := fs.String("message", "update", "commit message")
	fs.Parse(argv)
	if *repo == "" || *path == "" || *file == "" || *tokenFile == "" {
		fmt.Fprintln(os.Stderr, "github-put: -repo, -path, -file, -token-file are required")
		os.Exit(2)
	}
	tokenBytes, err := os.ReadFile(*tokenFile)
	if err != nil {
		fmt.Fprintln(os.Stderr, "github-put: token-file:", err)
		os.Exit(2)
	}
	token := strings.TrimSpace(string(tokenBytes))
	content, err := os.ReadFile(*file)
	if err != nil {
		fmt.Fprintln(os.Stderr, "github-put: file:", err)
		os.Exit(2)
	}

	url := fmt.Sprintf("%s/repos/%s/contents/%s", apiBase, *repo, *path)
	sha := currentSha(url, *branch, token)

	payload := map[string]interface{}{
		"message": *message,
		"content": base64.StdEncoding.EncodeToString(content),
	}
	if sha != "" {
		payload["sha"] = sha
	}
	if *branch != "" {
		payload["branch"] = *branch
	}
	buf, _ := json.Marshal(payload)
	req, _ := http.NewRequest("PUT", url, bytes.NewReader(buf))
	setGithubAuth(req, token)
	req.Header.Set("Content-Type", "application/json")
	resp, err := client.Do(req)
	if err != nil {
		fmt.Fprintln(os.Stderr, "github-put: send:", err)
		os.Exit(1)
	}
	defer resp.Body.Close()
	fmt.Println(resp.StatusCode)
	if resp.StatusCode < 200 || resp.StatusCode >= 300 {
		body, _ := io.ReadAll(resp.Body)
		fmt.Fprintln(os.Stderr, "github-put:", strings.TrimSpace(string(body)))
		os.Exit(1)
	}
}

// currentSha: the blob sha of an existing file, or "" when it does not exist yet (a create).
func currentSha(url, branch, token string) string {
	if branch != "" {
		url += "?ref=" + branch
	}
	req, _ := http.NewRequest("GET", url, nil)
	setGithubAuth(req, token)
	resp, err := client.Do(req)
	if err != nil {
		return ""
	}
	defer resp.Body.Close()
	if resp.StatusCode != 200 {
		return ""
	}
	var meta struct {
		Sha string `json:"sha"`
	}
	if json.NewDecoder(resp.Body).Decode(&meta) != nil {
		return ""
	}
	return meta.Sha
}

func setGithubAuth(req *http.Request, token string) {
	req.Header.Set("Authorization", "Bearer "+token)
	req.Header.Set("Accept", "application/vnd.github+json")
	req.Header.Set("User-Agent", "xnet")
}

func applyHeaderFile(req *http.Request, path string) {
	data, err := os.ReadFile(path)
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
