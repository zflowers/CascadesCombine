
#include "JSONFactory.h"
#include "BuildFit.h"


int main(){
	
	JSONFactory* j = new JSONFactory("test.json");
	BuildFit* BF = new BuildFit();
	BF->BuildAsimovFit(j);
}
