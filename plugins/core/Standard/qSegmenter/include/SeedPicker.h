#ifndef SEED_PICKER_H
#define SEED_PICKER_H

#include <ccMainAppInterface.h>
#include <ccPickingListener.h>
#include <ccPointCloud.h>
#include <vector>

class SeedPicker : public ccPickingListener
{
  public:
	// Constructor needs the app interface to access the picking Hub and Console
	SeedPicker(ccMainAppInterface* app);
	~SeedPicker() override;

	// to start and stop intercepting clicks
	void startListening();
	void stopListening();

	// The core method overridden from ccPickingListener
	void onItemPicked(const PickedItem& pi) override;

	// Retrieve the seeds later
	const std::vector<unsigned>& getPositiveSeeds() const
	{
		return m_positiveSeeds;
	}
	//positive/negative logic 
	void setPositiveMode(bool isPositive)
	{
		m_isPositive = isPositive;
	}
	const std::vector<unsigned>& getNegativeSeeds() const //this fucntion does not alter the state of the class, 
	{
		return m_negativeSeeds;
	}

  private:
	ccMainAppInterface*   m_app;
	std::vector<unsigned> m_positiveSeeds;

	// Optional: Keep track of the specific cloud we are segmenting
	ccPointCloud* m_targetCloud;

	bool                  m_isPositive = true;
	std::vector<unsigned> m_negativeSeeds;
};

#endif // SEED_PICKER_H