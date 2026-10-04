import { fileURLToPath } from "node:url";
import { defineConfig } from "astro/config";
import { remarkAlert } from "remark-github-blockquote-alert";

// Docs live at the repo root (../docs), outside the Astro project root, so Vite's
// file watcher never sees them and edits to docs/**/*.md don't hot reload in dev.
// Add the docs directory to the dev watcher so content edits invalidate and refresh.
const docsDir = fileURLToPath(new URL("../docs", import.meta.url));

function watchExternalDocs() {
  return {
    name: "crosssmudge:watch-external-docs",
    apply: "serve",
    configureServer(server) {
      server.watcher.add(docsDir);
    },
  };
}

function rewriteMarkdownLinks() {
  return (tree) => {
    visit(tree, (node) => {
      if (node.type !== "link" && node.type !== "image") {
        return;
      }

      if (typeof node.url !== "string") {
        return;
      }

      const rootDocUrl = node.url
        .replace(/^(\.\.\/)+SCOPE\.md(#.*)?$/i, "https://github.com/Mumfee/CrossSmudge/blob/main/SCOPE.md$2")
        .replace(/^(\.\.\/)+GOVERNANCE\.md(#.*)?$/i, "https://github.com/Mumfee/CrossSmudge/blob/main/GOVERNANCE.md$2");
      if (rootDocUrl !== node.url) {
        node.url = rootDocUrl;
        return;
      }

      node.url = node.url.replace(/(^|\/)README\.md(#.*)?$/i, "$1index.html$2").replace(/\.md(#.*)?$/i, ".html$1");
    });
  };
}

function visit(node, visitor) {
  visitor(node);
  if (!Array.isArray(node.children)) {
    return;
  }
  for (const child of node.children) {
    visit(child, visitor);
  }
}

export default defineConfig({
  build: {
    format: "file",
  },
  markdown: {
    remarkPlugins: [remarkAlert, rewriteMarkdownLinks],
  },
  vite: {
    plugins: [watchExternalDocs()],
  },
  site: "https://mumfee.github.io/CrossSmudge",
});
