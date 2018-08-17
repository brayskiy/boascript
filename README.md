# BoaScript is very light and easy scripting language or powerful calculator with C-style syntax. 

![Boascript](image/boascript-web.png) 

## Repository Structure

* [APP](src) Boascript implementation.
* [EXTRAS](extras) External classes.
* [TEST](test) Test suite

## Getting Started
```
	std::string test = "x = 0; b = 2; s = (b - x) / 500; while (x <= b) { y = a; x +=s; } println x;"

 	Boascript bs;
 	std::string& res = bs.Calc(test);
 	std::cout << res << std::endl;

```

## Language Reference

[Boascript Reference](https://github.com/brayskiy/boascript/doc/reference.html) 
