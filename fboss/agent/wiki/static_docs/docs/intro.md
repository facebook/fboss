---
slug: /
sidebar_position: -1
sidebar_label: Home
title: Welcome to FBOSS Agent docs
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

# Welcome to the FBOSS Agent docs

## Why static docs?

With the deprecation of `fb-sphinx`, it is becoming challenging to build and edit wikis. Particularly, any wiki pages stored in source code are going stale since the sphinx jobs must be run manually.

Docusaurus, this solution, is a pain to set up, but is pretty self sustaining once everything is working, with no need to worry about our docs not being built. It is all controlled through our own `npm` project that publishes these folders, so no worrying about some regular tooling needing to build this, our docs is our own service.

Also, now that this is set up with some magic fb internal plugins, there is no need to revisit the IDE to make page edits. You can edit the wiki (which will commit to source code) directly from the UI, making adding new pages much easier.

## Contributing through the UI

You should be able to contribute directly from this page by clicking **Edit this page** or **Add new page** in the bottom

Trying to edit in the IDE is fine, but its complicated to set up on the devserver side. Only edit in the actual source if you need to reorganize or the actual configuration needs to change.

:::tip

Staticdocs supports MDX, which is an extension to standard Markdown that has tons of useful features. Please check out the ["Markdown Features Page"](https://www.internalfb.com/intern/staticdocs/staticdocs/docs/documenting/markdown-features/) for everything that is available.

:::

## I want to edit in the IDE / I need to change something in the `staticdocs` environment!

Here is the process then:

### Prerequisite: Add `npm` to the path

:::note

Please ensure that you are not installing any new packages when running this. If your diff installs something new to `xplat` or another repo, its likely you messed something up. Everything you need is already stored in repo and you should just be pointing your path to the correct place here.

:::

At the bare minimum, you will need to ensure that `npm` is in your path. You just need to add this to your `$HOME/.bashrc` <Highlight>**on your devserver**</Highlight>

```bash
export PATH=$HOME/fbsource/xplat/third-party/node/bin:$PATH
```

### Prerequisite: Add `yarn` to the path

`yarn` is a much more easy way to administrate node libaries. I highly recommend it over `npm`.
Once `npm` is installed, its very easy. You just need to add this to your `$HOME/.bashrc` <Highlight>**on your devserver**</Highlight>

```bash
export PATH=$HOME/fbsource/xplat/third-party/yarn/:$PATH
```

### Step 1: Ensure you have the proper proxy settings


Start by running this:

```bash
sudo feature install ttls_fwdproxy && sudo chefctl -i
```

Then from the root directory of the project `FBSOURCE:fbcode/dc_push/dc_push_docs/`:
```bash
yarn config set proxy http://fwdproxy:8080
yarn config set https-proxy http://fwdproxy:8080
```

### Step 2: Create a Sandbox link to your devserver

You will need to run this command <Highlight>**from your laptop**</Highlight>. Just run this command and leave it running, this will tunnel the session out. Otherwise, you will not be able to connect to any sessions.
```
ssh -L 3000:localhost:3000 $USER.sb
```
:::tip
If you have support for `et` on your devserver, it is recommended to use that instead, as it will be a much more stable connection than the typical ssh forwarding

```bash
et $USER.sb:8080 -t 3000:3000
```
:::


### Step 3: Build with `yarn`

From the root directory of the project `FBSOURCE:fbcode/dc_push/dc_push_docs/`
```bash
yarn
yarn build-fb
```

### Step 4: Deploy and Serve your content

```bash
yarn serve
```

If you want to use npm to deploy and serve your content, use the below command.
```bash
npm run serve
```


If you followed these steps correctly, you will have everything at `localhost:3000`, or whatever link `yarn` reports.

## I want to add an Image

Regular Markdown images are supported. Add the image file to the folder `static/img/` or pull it from the pixelcloud, and it's all the same to static docs.

When you're on the local terminal, use the below command to copy the image from your local machine to the devserver.

```python
$ scp <path on local machine to file> <your_devserver_hostname>:<path on devserver>
```

## I want to look at the syntax of mdx

For examples and syntax on using mdx, refer to [Markdown Features](https://staticdocs.internalfb.com/staticdocs/docs/documenting/markdown-features/) page
