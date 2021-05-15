# @coalpha/ps_list

![](misc/icon.png)

*Efficiently list open processes in Node*

## usage

```js
const ps_list = require("@coalpha/ps_list");

ps_list().forEach(process => console.log(process.name));
```
