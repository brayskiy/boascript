# BoaScript is very light and easy scripting language or powerful calculator with C-style syntax. 

![Boascript](image/boascript-web.png) 

## Repository Structure

* [APP](src) Boascript implementation.
* [DISTRIBUTION](distribution) Static library and header.
* [TEST](test) Test suite

##Getting Startted
```
	std::string test = "x = 0; b = 2; s = (b - x) / 500; while (x <= b) { y = a; x +=s; } println x;"

 	Boascript bs;
 	std::string& res = bs.Calc(test);
 	std::cout << res << std::endl;

```

## Language Reference

![Boascript](file:///reference.html) 
