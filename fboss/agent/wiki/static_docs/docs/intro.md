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
