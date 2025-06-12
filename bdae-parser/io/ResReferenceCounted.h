#ifndef __RESIREFERENCECOUNTED_H__
#define __RESIREFERENCECOUNTED_H__

// Utility class for tracking file references in memory.
// _____________________________________________________

class IResReferenceCounted
{
public:
	//! increases the counter
	void grab() const
	{
		++ReferenceCounter;
	}

	//! decreases the counter and deletes the object if no references remain
	bool drop() const
	{
		--ReferenceCounter;

		if (ReferenceCounter == 0)
		{
			delete this;
			return true;
		}

		return false;
	}

protected:
	mutable unsigned int ReferenceCounter;

	IResReferenceCounted(bool startCountAtOne = true)
		: ReferenceCounter(startCountAtOne ? 1 : 0) {}

	virtual ~IResReferenceCounted() {}
};

#endif
