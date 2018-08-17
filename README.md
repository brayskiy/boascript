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

## Dependencies

To build the Boascript librtary [ooyacc](https://github.com/brayskiy/ooyacc) utility should be installed. 


## Language Reference

[Boascript Reference](doc/reference.md) 


## Applications using Boascript

[Boascript](https://play.google.com/store/apps/details?id=boris.boascript)

[Calclab](https://play.google.com/store/apps/details?id=boris.calclab.free) is a graphing calculator (also graphics / graphic calculator) refers to a class of scientific calculators that are capable of plotting graphs (Cartesian two and three dimensional, Polar and Parametric) and performing calculations using BoaScript scripting language. BoaScript has a C-style syntax and allows to conduct the calculations in the wide range - from arithmetic and trigonometric calculations to complex numerical solutions.