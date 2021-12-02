.. fbmeta::
    hide_page_title=true

Fboss Platform Wiki Pages Build
################################

Following is a description of how to build wiki pages for the FBOSS Platform team.

fbSphinx Setup on devserver
----------------------------

Run the following command to install fbSphinx on a devserver::

    % sudo feature install fbsphinx

New fbSphinx Directory Setup
-----------------------------

This section describes how to add a new directory to contain wiki pages for the QSFP service.
This instruction is also applicable to the Platform services, except the actual directory paths.
The example below shows how to add a new fbsource/fbcode/fboss/qsfp_service/wiki/concepts/features/ directory.
Note the convention is to have facebook specific and open source wikis reside in facebook/ and oss/ sub-directories, respectively.

1.  Add new directories.
::

    % cd fbsource/fbcode
    % mkdir fboss/qsfp_service/wiki/concepts/features
    % mkdir fboss/qsfp_service/wiki/concepts/features/facebook
    % mkdir fboss/qsfp_service/wiki/concepts/features/oss

2.  Add new TARGETS files. Note the TARGETS files of both facebook/ and oss/ sub-directories are identical
in this case because they have the same wiki_root_path.
::

    % vim fboss/qsfp_service/wiki/concepts/features/facebook/TARGETS
    load("@fbcode_macros//build_defs:sphinx_wiki.bzl", "sphinx_wiki")

    sphinx_wiki(
        name = "feature_docs",
        srcs = glob(["*.rst"]),
        wiki_root_path = "FBOSS_Platform/Features",
    )

    % vim fboss/qsfp_service/wiki/concepts/features/oss/TARGETS
    Its content is the same as above.

3.  Add new .rst files that contain new wiki pages (e.g. optics.rst, xphy.rst). An example of a .rst file is shown below.
::

    .. fbmeta::
        hide_page_title=true

    Optics
    #################

    Feature Description
    -------------------

    Overview
    ~~~~~~~~

4.  Add a new index.rst file if it does not already exist in a directory.
This file serves a wiki page that contains hyperlinks to all other .rst files in the same directory.
Add all new .rst file names to this index.rst file.
::

    % vim fboss/qsfp_service/wiki/concepts/features/facebook/index.rst

    .. fbmeta::
        hide_page_title=true

    .. toctree::
        :maxdepth: 1

        optics
        xphy

Preview New Wiki Content in EverPaste
--------------------------------------

When previewing wiki pages in EverPaste,they are published to the specified wiki paths and available to others to view and search for 30 days.
To delete a page manually, go to that page, click the ... in its upper right hand corner, and select delete.::

    % fbsphinx preview -e //fboss/qsfp_service/wiki/concepts/features/facebook:feature_docs

Note feature_docs in the above command is the name specified in the fboss/qsfp_service/wiki/concepts/features/facebook/TARGETS file.
When fbSphinx finishes building, it prints a url. Paste that link into a browser to preview wiki pages.

Publish New Wiki Content
-------------------------

New wiki content can be manually published from devserver.::

    % fbsphinx publish //fboss/qsfp_service/wiki/concepts/features/facebook:feature_docs

References
-----------
1. `FBOSS agent docathon recording <https://fb.workplace.com/100065466012089/videos/924245448167149>`_.
2. `FBOSS agent docathon notes <https://fb.quip.com/anakACwq4BVJ>`_.
3. `fbSphinx overview <https://docs.google.com/presentation/d/14fOuismbn3nZsPtC7Uc-ZI6AIh9NIvg0/edit#slide=id.p5>`_.
