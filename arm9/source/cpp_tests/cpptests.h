#ifndef HEADER_H_
#define HEADER_H_

#include <stdio.h>
#include <vector>
#include <string>

namespace space1
{
	typedef struct
	{
		int foo;
	} booExtern;
}

// one template class used for simple template parameters
template <typename T>
class Foo {
public:
  T i;
};

typedef int Int32;

struct Youpi
{
  int blo;
};

struct Bouba
{
  double foo;
};

namespace space1
{
	typedef int boo;

	// some overloaded C style functions
	int foo1();
	void foo1(int x);
	char foo1(char j);
}

namespace space1
{
	// a struct inside a namespace
	typedef struct
	{
		int foo;
	} oneStruct;

  class Class1
  {
  public:
	int attribute1;
	
    // a constructor and an overload
    Class1() { };

    Class1(int i, float j) { };

    // one destructor
    ~Class1() { };

    int func1(int i, int j);

    // an overloaded function. Defines 2 times the same symbol!
    int func1(float a, char *b);

    // one function that should be removed, being in the exclusion list
    char funcToRemove(int stuff);

    // another function that should be removed
    void funcToRemoveTwo(int stuff, float bla);
    
    // a very complex signature : passing a pointer to function
    // as a parameter. Note that I've given name to the parameters,
    // but the original code had a mix of anonymous parameters (K&R style) and names parameters
    int ldpNHResRoute_Walk(double vrId,
			   char prefixType,
			   void **ppNHResRoute,
			   Int32 ** (*pWalkFn)(char * bla,
					       void * boo,
					       void * foo),
			   void *pArg1,
			   void *pArg2);
    
    struct Youpi *funcFunc(void (*toto)(int bla));
    void *funcFunc2(struct Bouba * (*titi)(int blu));
    
    // a signature with a template parameter
    //int ** funcWithTemplateParam (vector < vector <int *> > param1, stuff<foo> param2);

    // a signature taking references
    char takesReferences(int & bla, float & foo);

    // a signature returning a reference
    int & returnReference(float bidule);

    // a signature taking an array parameter
    void withArray(char bla []);

    // a signature taking a template parameter
    // as a reference. Passing directly the object is possible
    // but then I can't compare it as I don't have a != overload
    int functionWithTParam(int j, Foo<float> & foo);

	// one function taking as parameter a type defined in the same namespace,
	// but not fully scoped in the prototype. The generated code should scope properly the type
	// in the callback declaration as the callback declaration is not in the namespace.
	int functionWithStruct(oneStruct *par, boo par2, booExtern par3);
	
	int multiply(int a, int b){
		return (a*a);
	}
	
  };
}

// a simple C like function with references
int & returnReferenceAsC(float & parm1);

// a C like function in a namespace
namespace n1
{
namespace n2
{
	char funcInNamespace(char u, float v);
}
}

typedef char Boolean;

namespace OTDRProxy
{
  typedef struct {
    int bla;
  } T_ConfigLine;
  
  typedef struct {
    char *foo;
  } InvocationAsync;

  class Configuration_ske {
  public:
    int boo;
  };
  
  class OTDRConfigurationItf : public virtual OTDRProxy::Configuration_ske 
  {
  public:
    
    OTDRConfigurationItf () {}
    ~OTDRConfigurationItf () {}
    
    // Operation ConfigLine
    virtual void ConfigLine(const OTDRProxy::T_ConfigLine& configLine,
  			    Boolean& result,
  			    OTDRProxy::InvocationAsync& invocation);  
  protected:
    
  private:
    
    // copy constructor
    OTDRConfigurationItf (OTDRConfigurationItf const &other);
    // assignment operator
    OTDRConfigurationItf& operator=(OTDRConfigurationItf const &other);
  };
}

// A function template, outside of namespace
// with some references to spice up
template <typename X, typename Y>
X templateFunctionNoNamespace_renamed(X &x, Y y)
{
  return x + y;
}

// A function template, inside nested namespace
namespace ftemp1
{
  namespace ftemp2
  {
    template <typename U, typename V>
    U templateFunctionWithNamespace(U &x, V y)
    {
      return x * y;
    }
  }
}

// A simple class template
namespace template1
{
template <typename T, typename V>
class TemplateB {
 public:
  void yahoo(T & param, V *param2) {
    printf("Hello World\n");
  }
};
}

// a somewhat complex template template class
/*
template <typename T, typename U=float, template <typename X=int, typename Y=int> class TemplateB>
  class TemplateA {
 private:
 U parameterU;
 T * parameterT;
 TemplateB * <X, Y> parameterV;
 public:
 T & operation1(T & param1, U * param2, TemplateB * <X, Y> param3) {
   parameterU = *param2;
   parameterT = param1;// must have a copy constructor
   parameterV = param3;
   return param1;
 }
};
*/

// one global function, C style, with a default parameter value, and an extern qualifier
// this should result in 2 overloads in the generated code
extern int withDefaultValue(int j, float X=1.0);

// several overloads of the same global C style function
int functionOverload (int i, char j);
int functionOverload (int i, char j, void *z);
int functionOverload (int i, char j, float t);

// a class with a constructor accepting default values for parameters
namespace sunny
{
class DefaultParametersInConstructor
{
public:
	DefaultParametersInConstructor(int val1, int val2=0, char *val3=NULL, float val4=0.0);

    //same thing but for an operation with optional parameters
	void methodOptionalParameters(int val1, int val2 = 2);
};
}

//a C style function with optional parameters
int cstyleWithOptionalParameter(float a, float b=1.0, int c=2);

int fiveOptionalParameter(float a, float b, int i1=1, int i2=2, int i3=3, int i4=4, int i5=5);

namespace tryit
{
	int cstyleWithOptionalParameter(float a, float b=1.0, int c=2);
}

typedef int T_dataType;

// an operation with a trailing const
class rainy
{
public:
    //in the generated code I actually need to append the const as well!
    int anotherConstFunction(int *nb) const;
	unsigned long withData(const T_dataType dataType);
	// a function being const and returning const
	const unsigned long doubleConst(int data) const;
	//a function returning a Vector of something
	std::vector<std::string> vectorOperation(int i);
private:
	//a private operation. Should I mock private operations?
	const char * privateReturnsConst(int i);
};

//a class with no operations. Should not result in generated code.
class DontGenerateMe
{
	public:
	int foo;
};


#endif

#ifdef __cplusplus
extern "C" {
#endif

extern void test_mock_1();
extern void test_mock_2();
extern void test_mock_3();
extern void test_mock_4();
extern void test_mock_5();
extern void test_mock_6();
extern void test_mock_7();

extern int doCppTests();

#ifdef __cplusplus
}
#endif