# @coalpha/ps_list

![](misc/icon.png)

*Efficiently list open processes in Node*

This repository is incomplete and archived because making a faster alternative for
[ps-list](https://github.com/sindresorhus/ps-list) doesn't actually make sense. If you want something performant, don't use Node.js.

## usage

```js
const ps_list = require("@coalpha/ps_list");

ps_list().forEach(process => console.log(process.name));
```
