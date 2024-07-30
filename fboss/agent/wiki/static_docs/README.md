# Website

This website is built using [Docusaurus 2](https://docusaurus.io/), a modern static website generator.

## Preview MarkdownX files

To preview mdx files in Visual Studio Code, you can enable the built-in Markdown Preview to be used on `.mdx` file extensions: https://stackoverflow.com/a/75035795

To open the Markdown Preview pane for an mdx file, press `⌘ + k`, then `v`.

## Installation

Navigate to this directory:
```bash
cd ~/fbsource/fbcode/fboss/agent/wiki/static_docs
```

Set up the libraries:
```bash
yarn
```

## Local Development

Compile the documentation and start a local development server:
```bash
yarn start
```

You can preview the pages at localhost:3000.

If you are running this command from a devserver, you will need to port forward your local port to the devserver to be able to view the page in your browser. Re-initiate the ssh connection to your devserver using the following ssh command:
```bash
ssh -L 3000:localhost:3000 YOUR_DEVSERVER_URI
```

Most changes will be reflected live without having to restart this server.


## Build

```bash
yarn build
```

This command generates static content into the `build` directory and can be served using any static contents hosting service.
