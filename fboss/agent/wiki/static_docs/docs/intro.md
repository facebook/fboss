---
slug: /
sidebar_position: -1
sidebar_label: Home
title: Home
---

export const Highlight = ({children, color}) => (
  <span
    style={{
      backgroundColor: '#fdfd96',
      borderRadius: '2px',
      color: '#000',
      padding: '5px',
      cursor: 'pointer',
    }}
  >
    {children}
  </span>
);


# Static Docs

This website is built using [Docusaurus 2](https://docusaurus.io/), a modern static website generator.


## Why static docs?

With the deprecation of `fb-sphinx`, it is becoming challenging to build and edit wikis. Particularly, any wiki pages stored in source code are going stale since the sphinx jobs must be run manually.

Docusaurus, this solution, is a pain to set up, but is pretty self sustaining once everything is working, with no need to worry about our docs not being built. It is all controlled through our own `npm` project that publishes these folders, so no worrying about some regular tooling needing to build this, our docs is our own service.

Also, now that this is set up with some magic fb internal plugins, there is no need to revisit the IDE to make page edits. You can edit the wiki (which will commit to source code) directly from the UI, making adding new pages much easier.


# Static Docs: Contributing

## Contributing through Code
All our wiki pages are hosted under ~/fbsource/fbcode/fboss/agent/wiki/static_docs/docs/.
* Depending on where you want the wiki to be placed under in the sidebar, choose the appropriate folder
* Once selected, create a new MDX file under the directory for OSS page or under fb/ for closed source page
* Once you created the new MDX page, refer to the instructions below to setup your devserver to view the changes
## Contributing through the UI

You should be able to contribute directly from this page by clicking **Edit this page** or **Add new page** in the bottom.

Trying to edit in the IDE is fine, but its complicated to set up on the devserver side. Only edit in the actual source if you need to reorganize or the actual configuration needs to change.

:::tip
Staticdocs supports MDX, which is an extension to standard Markdown that has tons of useful features. Please check out the ["Markdown Features Page"](https://www.internalfb.com/intern/staticdocs/staticdocs/docs/documenting/markdown-features/) for everything that is available.
:::


## I want to edit in the IDE / I need to change something in the `staticdocs` environment!

Here is the process then:

### Step 0: Preview MarkdownX files

To preview mdx files in Visual Studio Code, you can enable the built-in Markdown Preview to be used on `.mdx` file extensions: https://stackoverflow.com/a/75035795

To open the Markdown Preview pane for an mdx file, press `⌘ + k`, then `v`.


### Step 1: Forward Port to Devserver

You will need to port forward your local port to the devserver to be able to view the compiled page in your browser. <Highlight>**From your laptop**</Highlight>, initiate the ssh connection to your devserver using the following ssh command:
```bash
ssh -L 3000:localhost:3000 YOUR_DEVSERVER_URI
```

Run this command and leave it running, this will tunnel the session out. Otherwise, you will not be able to connect to the webpage.

:::tip
If you have support for `et` on your devserver, it is recommended to use that instead, as it will be a much more stable connection than the typical ssh forwarding:
```bash
et $USER.sb:8080 -t 3000:3000
```
:::


### Step 2: Clone the FBSOURCE Repository

If you have not already cloned the `fbsource` repository, do so in your devserver:
```bash
fbclone fbsource
```

Navigate to this directory (the root directory of the project):
```bash
cd fbsource/fbcode/fboss/agent/wiki/static_docs
```

### Step 2: Build with `yarn`

Set up the libraries:
```bash
yarn
```

### Step 3: Deploy and Serve your content

Compile the documentation and start a local development server:
```bash
yarn start
```

You can preview the pages at http://localhost:3000.

Most changes will be reflected live without having to restart this server.


### Step 4: Build Changes

Before committing your changes, run:
```bash
yarn build
```

This command generates static content into the `build` directory and can be served using any static contents hosting service.



## I want to add an Image

Regular Markdown images are supported. Add the image file to the folder `static/img/` or pull it from the pixelcloud, and it's all the same to static docs.

When you're on the local terminal, use the below command to copy the image from your local machine to the devserver.

```bash
scp <path on local machine to file> <your_devserver_hostname>:<path on devserver>
```

## I want to look at the syntax of mdx

For examples and syntax on using mdx, refer to [Markdown Features](https://staticdocs.internalfb.com/staticdocs/docs/documenting/markdown-features/) page.
