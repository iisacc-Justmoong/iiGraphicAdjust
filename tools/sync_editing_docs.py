#!/usr/bin/env python3
"""Create lossless agent projections for this change's three source documents.

The workspace-wide generator is absent from this checkout. This deliberately
narrow fallback preserves the same schema and records its distinct version;
it does not rewrite or reorder any unrelated document or existing index entry.
"""
import hashlib
import json
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
PROJECT = {"id": "urn:iisacc:project:SDK%2FiiGraphicAdjust", "name": "iiGraphicAdjust",
           "root": "SDK/iiGraphicAdjust", "kind": "sdk"}
DOCUMENTS = (("AGENT.md", "docs/Entrypoints/AGENT.agent.json", "instructions", "ko"),
             ("README.md", "docs/Entrypoints/README.agent.json", "overview", "en"),
             ("docs/AdvancedEditing.md", "docs/AdvancedEditing.agent.json", "guide", "ko"))
LIST = re.compile(r"^\s*([-*+]|\d+[.)])\s+(.*)")
HEADING = re.compile(r"^(#{1,6})\s+(.*?)\s*$")
LINK = re.compile(r"(?<!!)\[([^\]]+)\]\(([^)]+)\)")
CHECK = "--check" in sys.argv


def write_or_check(path, content):
    if CHECK:
        if not path.exists() or path.read_text(encoding="utf-8") != content:
            raise SystemExit(f"Stale agent projection: {path}")
    else:
        path.write_text(content, encoding="utf-8")


def blockify(lines):
    blocks = []
    i = 0
    while i < len(lines):
        start = i
        line = lines[i]
        heading = HEADING.match(line.rstrip("\n"))
        fence = re.match(r"^\s*```([^\s`]*)\s*$", line.rstrip("\n"))
        item = LIST.match(line)
        extra = {}
        if not line.strip():
            kind = "blank"
            while i < len(lines) and not lines[i].strip(): i += 1
        elif heading:
            kind = "heading"; i += 1
            extra = {"level": len(heading.group(1)), "title": heading.group(2)}
        elif fence:
            kind = "fenced_code"; i += 1; code = []
            while i < len(lines) and not re.match(r"^\s*```\s*$", lines[i].rstrip("\n")):
                code.append(lines[i]); i += 1
            closed = i < len(lines)
            if closed: i += 1
            extra = {"language": fence.group(1), "code": "".join(code), "closed": closed}
        elif item:
            kind = "list"; items = []
            while i < len(lines) and lines[i].strip():
                match = LIST.match(lines[i])
                if match:
                    items.append({"marker": match.group(1), "text": match.group(2).strip(), "checked": None})
                elif items and (lines[i].startswith(" ") or lines[i].startswith("\t")):
                    items[-1]["text"] += " " + lines[i].strip()
                else:
                    break
                i += 1
            extra = {"items": items}
        elif line.lstrip().startswith("<!--") or line.lstrip().startswith("<a "):
            kind = "html"; i += 1
        else:
            kind = "paragraph"; i += 1
            while i < len(lines) and lines[i].strip() and not HEADING.match(lines[i].rstrip("\n")) and not LIST.match(lines[i]) and not lines[i].lstrip().startswith(("```", "<!--", "<a ")):
                i += 1
        block = {"id": f"b{len(blocks)+1:06}", "kind": kind, "start_line": start+1,
                 "end_line": i, "raw": "".join(lines[start:i])}
        block.update(extra); blocks.append(block)
    return blocks


def convert(source, target, role, language):
    raw = (ROOT/source).read_bytes(); text = raw.decode("utf-8")
    lines = text.splitlines(keepends=True); blocks = blockify(lines)
    if "".join(block["raw"] for block in blocks) != text:
        raise ValueError("Projection lost source text: " + source)
    previous = json.loads((ROOT/target).read_text()) if (ROOT/target).exists() else {}
    doc_id = previous.get("document_id", "urn:iisacc:document:" + hashlib.sha256((PROJECT["root"]+"/"+source).encode()).hexdigest())
    outline = []; ancestors = []; links = []; commands = []; directives = []
    for block in blocks:
        if block["kind"] == "heading":
            while ancestors and ancestors[-1]["level"] >= block["level"]: ancestors.pop()
            anchor = re.sub(r"[^\w\- ]", "", block["title"].lower()).strip().replace(" ", "-")
            entry = {"block_id": block["id"], "parent_id": ancestors[-1]["block_id"] if ancestors else None,
                     "level": block["level"], "title": block["title"], "anchor": anchor,
                     "start_line": block["start_line"]}
            outline.append(entry); ancestors.append(entry)
        if block["kind"] == "fenced_code" and block["language"] in ("sh", "bash", "zsh", "shell"):
            commands.append({"block_id": block["id"], "language": block["language"], "code": block["code"]})
        if block["kind"] not in ("fenced_code", "html"):
            for offset, line in enumerate(block["raw"].splitlines()):
                for match in LINK.finditer(line):
                    dest = match.group(2)
                    kind = "anchor" if dest.startswith("#") else "external" if "://" in dest else "relative"
                    links.append({"block_id": block["id"], "label": match.group(1), "target": dest,
                                  "kind": kind, "line": block["start_line"]+offset})
                if re.search(r"\b(?:must|required|prohibit|forbid)\b|반드시|금지|필수", line, re.I):
                    directives.append({"block_id": block["id"], "line": block["start_line"]+offset,
                                       "text": line.strip(), "basis": "lexical_match", "context": block["kind"]})
    digest = hashlib.sha256(raw).hexdigest()
    projection = {"format": "iisacc.agent-document", "schema_version": 1,
                  "document_id": doc_id, "project": PROJECT,
                  "source": {"path": source, "sha256": digest, "bytes": len(raw),
                             "line_count": len(lines), "encoding": "utf-8", "language": language},
                  "document": {"title": outline[0]["title"] if outline else source,
                               "role": role, "authority": "markdown_source"},
                  "blocks": blocks, "outline": outline,
                  "indexes": {"directive_candidates": directives, "commands": commands, "links": links, "tables": []},
                  "provenance": {"generator": "workspace-docs", "version": "compat-fallback-1",
                                 "profile": "lossless-blocks-v1", "derived": True}}
    write_or_check(ROOT/target, json.dumps(projection, ensure_ascii=False, indent=2)+"\n")
    return {"document_id": doc_id, "source": source, "agent_json": target,
            "source_sha256": digest, "role": role}


index_path = ROOT/"docs/agent-index.json"
index = json.loads(index_path.read_text(encoding="utf-8"))
updates = [convert(*entry) for entry in DOCUMENTS]
for update in updates:
    existing = next((i for i, entry in enumerate(index["documents"]) if entry["source"] == update["source"]), None)
    if existing is None:
        position = next((i for i, entry in enumerate(index["documents"])
                         if entry["source"].startswith("docs/") and entry["source"] > update["source"]), len(index["documents"]))
        index["documents"].insert(position, update)
    else:
        index["documents"][existing] = update
write_or_check(index_path, json.dumps(index, ensure_ascii=False, indent=2)+"\n")

# Preserve the separately maintained pre-existing Architecture projection while
# checking the two dependency-policy corrections made for this change.
architecture = json.loads((ROOT/"docs/Architecture.agent.json").read_text(encoding="utf-8"))
architecture_source = (ROOT/"docs/Architecture.md").read_bytes()
architecture_digest = hashlib.sha256(architecture_source).hexdigest()
architecture_entry = next(entry for entry in index["documents"] if entry["source"] == "docs/Architecture.md")
if (architecture["source"]["sha256"] != architecture_digest or
        architecture_entry["source_sha256"] != architecture_digest or
        "".join(block["raw"] for block in architecture["blocks"]).encode("utf-8") != architecture_source):
    raise SystemExit("Architecture Markdown and its existing agent projection are not synchronized")
