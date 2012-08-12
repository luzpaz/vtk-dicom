#include "vtkDICOMSequence.h"
#include "vtkDICOMSequenceItem.h"
#include "vtkDICOMDictionary.h"

#include <sstream>

#include <string.h>
#include <stdlib.h>

// macro for performing tests
#define TestAssert(t) \
if (!(t)) \
{ \
  cout << exename << ": Assertion Failed: " << #t << "\n"; \
  cout << __FILE__ << ":" << __LINE__ << "\n"; \
  cout.flush(); \
  rval |= 1; \
}

int main(int argc, char *argv[])
{
  int rval = 0;
  const char *exename = argv[0];

  // remove path portion of exename
  const char *cp = exename + strlen(exename);
  while (cp != exename && cp[-1] != '\\' && cp[-1] != '/') { --cp; }
  exename = cp;

  // some miscellaneous UIDs to use
  const char *classUID = "1.2.840.10008.5.1.4.1.1.4";
  const char *instanceUIDFormat =
    "1.2.840.113619.2.176.2025.4110284.7408.1276101323.%i";
  char instanceUID[54];

  // create the ReferencedSeriesSequence
  vtkDICOMSequence seq(1);

  // only add one item to the sequence
  for (int i = 0; i < 1; i++)
    {
    // add the first data element to the item
    vtkDICOMSequenceItem item;
    item.AddDataElement(DC::SeriesInstanceUID,
      vtkDICOMValue(vtkDICOMVR::UI,
        "1.2.840.113619.2.176.2025.4110284.7478.1276100777.239"));

    // the second data element will be a nested sequence
    vtkDICOMSequence seq2;

    // add ten items to this sequence
    for (int j = 0; j < 10; j++)
      {
      // create a unique InstanceUID
      sprintf(instanceUID, instanceUIDFormat, 255+j);
      vtkDICOMSequenceItem item2;
      item2.AddDataElement(DC::ReferencedSOPClassUID,
        vtkDICOMValue(vtkDICOMVR::UI, classUID));
      item2.AddDataElement(DC::ReferencedSOPInstanceUID,
        vtkDICOMValue(vtkDICOMVR::UI, instanceUID));
      seq2.AddItem(item2);
      }

    // create the ReferencedInstanceSequence from the items
    item.AddDataElement(DC::ReferencedInstanceSequence, seq2);

    // add this sequence-containing item to the original sequence
    seq.SetItem(i, item);
    }

  // do the tests
  unsigned int n = seq.GetNumberOfValues();
  TestAssert(n == 1);

  // use iterator to go through the elements in each item
  vtkDICOMDataElementIterator iter;

  // use a plain old pointer to go through items in sequence
  const vtkDICOMSequenceItem *ip1 = seq.GetSequenceData();

  // test variables
  bool found1 = false;
  bool found2 = false;
  int fullcount = 0;

  // go through the item in ReferencedSeriesSequence
  for (iter = ip1->GetData(); iter != ip1->GetDataEnd(); ++iter)
    {
    // make sure SeriesInstanceUID was found
    if (iter->GetTag() == DC::SeriesInstanceUID)
      {
      found1 = true;
      TestAssert(strcmp(iter->GetValue().GetTextData(),
        "1.2.840.113619.2.176.2025.4110284.7478.1276100777.239") == 0);
      }
    // make sure ReferencedInstanceSequence was found
    if (iter->GetTag() == DC::ReferencedInstanceSequence)
      {
      found2 = true;
      vtkDICOMDataElementIterator iter2;

      // get the nested sequence
      vtkDICOMValue v2 = iter->GetValue();
      const vtkDICOMSequenceItem *ip2 = v2.GetSequenceData();
      unsigned int m = v2.GetNumberOfValues();
      TestAssert(m == 10);

      // go through the 10 items in the sequence
      for (unsigned int j = 0; j < m; j++)
        {
        // check the two elements in each item
        TestAssert(ip2[j].GetNumberOfDataElements() == 2);
        iter2 = ip2[j].GetData();
        TestAssert(iter2->GetTag() == DC::ReferencedSOPClassUID);
        TestAssert(strcmp(iter2->GetValue().GetTextData(),
          "1.2.840.10008.5.1.4.1.1.4") == 0);
        ++iter2;
        TestAssert(iter2->GetTag() == DC::ReferencedSOPInstanceUID);
        sprintf(instanceUID, instanceUIDFormat, 255+j);
        TestAssert(strcmp(iter2->GetValue().GetTextData(),
          instanceUID) == 0);
        // make sure end of the element list has been reached
        ++iter2;
        TestAssert(iter2 == ip2[j].GetDataEnd());
        }
      }
    fullcount++;
    }

  // check that there were two elements in the ReferencedSeriesSequence item
  TestAssert(fullcount == 2);

  return rval;
}