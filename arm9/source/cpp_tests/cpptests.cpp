#include <stdio.h>
#include <iostream>
#include "opmock.h"
#include "cpptests.h"
#include "consoleTGDS.h"
#include "WoopsiTemplate.h"

class ToTest
{
private:
  space1::Class1 oneClass3;
public:
  int doSomething(int a);
};

int ToTest::doSomething(int a)
{
  int result = oneClass3.multiply(a, a);
  return result;
}

static int class3_callback (int  a, int  b, int calls, space1::Class1 * ptr)
{
  // perform a side effect on the class itself
  printfWoopsi("Calling class3_callback a=%d b=%d calls=%d val=%d", a, b, calls,ptr->attribute1);
  ptr->attribute1 = 42;
  return 1234567;
}





// the struct to store calls to this template function
// il me faut absolument la liste des parametres,
// depuis le header...
template <typename X, typename Y>
struct templateFunctionNoNamespace_call
{
  X * x;
  Y y;
  X return_value;
  char check_params;
};

// cette structure doit forcement etre instanciee
// au moment du test. Je ne peux pas la déclarer en static
// car je ne connais pas le type associé.
template <typename X, typename Y>
struct templateFunctionNoNamespace_struct
{
  int expectedCalls;
  int actualCalls;
  char useRealImpl;
  // can't use templates with typedefs
  // so specify the callback signature directly!
  X (* callback)(X & x, Y y, int calls);
  templateFunctionNoNamespace_call<X, Y> calls[MAX_FUNC_CALL];
};
templateFunctionNoNamespace_struct<double, float> templateFunctionNoNamespace_struct_inst;

///////////////
// these templates mock should be generated in a separate file!
// to include here verbatim and to used in the test.
//TODO toutes les fonctions sont a générer ici
//le fichier peut etre inclus plusieurs fois?



//TODO est-il utile de vouloir faire appel dans ce cas au callback original?
//je ne pense pas. On va reellement remplacer tout le comportement?
template <typename X, typename Y>
X templateFunctionNoNamespace(X &x, Y y)
{
  int opmock_i;
  X default_res = templateFunctionNoNamespace_struct_inst.calls[0].return_value = y;

  //TODO gerer champ callback
  //if (templateFunctionNoNamespace_struct_inst.callback != NULL)
  //  {
        templateFunctionNoNamespace_struct_inst.actualCalls++;
        //return do_sound_struct_inst.callback (sound, do_sound_struct_inst.actualCalls);
	//  }

  if (templateFunctionNoNamespace_struct_inst.expectedCalls == 0)
    {
      printfWoopsi("WARNING : unexpected call of templateFunctionNoNamespace");
	  printfWoopsi(" returning random value.");
      return default_res;
    }

  templateFunctionNoNamespace_struct_inst.actualCalls++;
  
  if (templateFunctionNoNamespace_struct_inst.calls[0].check_params == 1)
    {
      //attention x est une reference il faut donc comparer par
      //defaut les pointeurs
      //je suis obligé de les caster en void a cause de bizarreries
      // dans C++ (upcast implicite en double?)
      // il me faudrait vraiment des matchers pour rendre
      //le test utile (matcher float-float par ex;
      //lui passer 2 pointeurs sur void.
      if ((void *) templateFunctionNoNamespace_struct_inst.calls[0].x != (void *)&x)
        {
			printfWoopsi("WARNING : wrong value for parameter 'x' when calling templateFunctionNoNamespace (call %d)", templateFunctionNoNamespace_struct_inst.actualCalls);
        }

      if (templateFunctionNoNamespace_struct_inst.calls[0].y != y)
        {
			printfWoopsi("WARNING : wrong value for parameter 'y' when calling templateFunctionNoNamespace (call %d)", templateFunctionNoNamespace_struct_inst.actualCalls);
        }
    }

  for(opmock_i = 1; opmock_i < templateFunctionNoNamespace_struct_inst.expectedCalls; opmock_i++)
    {
      templateFunctionNoNamespace_struct_inst.calls[opmock_i - 1] = templateFunctionNoNamespace_struct_inst.calls[opmock_i];
    }
  
  templateFunctionNoNamespace_struct_inst.expectedCalls--;
  return default_res;
}

int doCppTests()
{
	printfWoopsi("Begin CPP Tests. ");
	
//TESTS1
  // initialize the mock class
  // this works for all instances, even instances I've not created 
  
  ToTest class1;
  int res = class1.doSomething(4);
  if(res != 16) {
	printfWoopsi("Failure in result");
  }
  res = class1.doSomething(4);
  printfWoopsi("Result 2 %d", res);
  
  // now use an instance callback - meaning that I need to get
  // a pointer on the class I want to mock
  space1::Class1 class3;
  class3.attribute1 = -777;

  int res2 = class3.multiply(2, 24);
  printfWoopsi("Result instance mock:%d", res2);
  printfWoopsi("attribute value in class:%d", class3.attribute1);

//TESTS2
	templateFunctionNoNamespace_struct_inst.expectedCalls = 32;

	float param1 = 2.0;
	double resDouble = templateFunctionNoNamespace<float, double> (param1, 3.0);
	if((3.0 - (resDouble - param1)) == param1){
		printfWoopsi("CPP 2 Test OK.");
	}
	else{
		printfWoopsi("CPP 2 Test ERROR.");
	}
	return 0;
}
