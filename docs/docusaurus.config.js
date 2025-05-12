/**
 * (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
 */

/* eslint-disable */

const lightCodeTheme = require('prism-react-renderer/themes/github');
const darkCodeTheme = require('prism-react-renderer/themes/dracula');

// With JSDoc @type annotations, IDEs can provide config autocompletion
/** @type {import('@docusaurus/types').DocusaurusConfig} */
(module.exports = {
  title: 'FBOSS Documentation', // TODO
  tagline: 'https://github.com/facebook/fboss', // TODO
  url: 'https://internalfb.com',
  baseUrl: '/fboss/',
  onBrokenLinks: 'throw',
  onBrokenMarkdownLinks: 'throw',
  trailingSlash: true,
  favicon: 'img/favicon.ico',
  organizationName: 'facebook',
  projectName: 'fboss', // TODO
  deploymentBranch: 'main',
  customFields: {
    fbRepoName: 'fbsource',
    ossRepoPath: 'fbcode/fboss/github/docs',
  },

  presets: [
    [
      'docusaurus-plugin-internaldocs-fb/docusaurus-preset',
      /** @type {import('docusaurus-plugin-internaldocs-fb').PresetOptions} */
      ({
        docs: {
          sidebarPath: require.resolve('./sidebars.js'),
          editUrl: 'https://www.internalfb.com/code/fbsource/fbcode/fboss/github/docs', // TODO Please change this to your repo.
        },
        experimentalXRepoSnippets: {
          baseDir: '.',
        },
        staticDocsProject: 'fboss',
        trackingFile: 'fbcode/staticdocs/WATCHED_FILES',
        blog: {
          showReadingTime: true,
          editUrl: 'https://www.internalfb.com/code/fbsource/fbcode/fboss/github/docs', // TODO change to path to your project in fbsource/www
        },
        theme: {
          customCss: require.resolve('./src/css/custom.css'),
        },
      }),
    ],
  ],

  themeConfig:
    /** @type {import('@docusaurus/preset-classic').ThemeConfig} */
    ({
      navbar: {
        title: 'fboss',
        logo: {
          alt: 'fboss Logo',
          src: 'img/logo.svg',
        },
        items: [
          {label: 'Build', position: 'left', docId: 'build/building_fboss_on_docker_containers', type: 'doc'},
          {label: 'Develop', position: 'left', docId: 'developing/platform_mapping', type: 'doc'},
          {label: 'Test', position: 'left', docId: 'testing/sensor_service_hw_test', type: 'doc'},
          // TODO: change docId when populated
          // {label: 'Contribute', position: 'left', docId: 'build/building_fboss_on_docker_containers', type: 'doc'},
          {label: 'Architecture', position: 'left', docId: 'architecture/meta_switch_architecture', type: 'doc'},
          {
            href: 'https://github.com/facebook/fboss',
            label: 'GitHub',
            position: 'right',
          },
        ],
      },
      footer: {
        style: 'dark',
        /*
        links: [
          {
            title: 'Docs',
            items: [
              {
                label: 'Tutorial',
                to: '/docs/intro',
              },
            ],
          },
          {
            title: 'Community',
            items: [
              {
                label: 'Stack Overflow',
                href: 'https://stackoverflow.com/questions/tagged/docusaurus',
              },
              {
                label: 'Discord',
                href: 'https://discordapp.com/invite/docusaurus',
              },
              {
                label: 'Twitter',
                href: 'https://twitter.com/docusaurus',
              },
            ],
          },
          {
            title: 'More',
            items: [
              {
                label: 'Blog',
                to: '/blog',
              },
              {
                label: 'GitHub',
                href: 'https://github.com/facebook/docusaurus',
              },
            ],
          },
        ],
        copyright: `Copyright © ${new Date().getFullYear()} My Project, Inc. Built with Docusaurus.`,
        */
        copyright: `Copyright © 2004-Present Facebook. All Rights Reserved.`,
      },
      prism: {
        theme: lightCodeTheme,
        darkTheme: darkCodeTheme,
      },
    }),
});
