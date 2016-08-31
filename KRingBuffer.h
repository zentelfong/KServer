#pragma once
#include "KMemPoll.h"
#include "KUtil.h"

//¶ÁÐ´Êý¾Ý»º´æ
class KRingBuffer
{
public:
	KRingBuffer(size_t capacity)
		: beg_index_(0)
		, end_index_(0)
		, size_(0)
		, capacity_(capacity)
	{
		data_ = (char*)kMalloc(capacity);
	}

	~KRingBuffer()
	{
		kFree(data_);
	}

	size_t Size() const { return size_; }
	size_t Capacity() const { return capacity_; }

	size_t Write(const char *data, size_t bytes)
	{
		if (bytes == 0) return 0;
		size_t capacity = capacity_;
		size_t bytes_to_write = kmin(bytes, capacity - size_);

		// Write in a single step
		if (bytes_to_write <= capacity - end_index_)
		{
			memcpy(data_ + end_index_, data, bytes_to_write);
			end_index_ += bytes_to_write;
			if (end_index_ == capacity) 
				end_index_ = 0;
		}
		// Write in two steps
		else
		{
			size_t size_1 = capacity - end_index_;
			memcpy(data_ + end_index_, data, size_1);
			size_t size_2 = bytes_to_write - size_1;
			memcpy(data_, data + size_1, size_2);
			end_index_ = size_2;
		}

		size_ += bytes_to_write;
		return bytes_to_write;
	}

	size_t Read(char *data, size_t bytes)
	{
		if (bytes == 0) return 0;
		size_t capacity = capacity_;
		size_t bytes_to_read = kmin(bytes, size_);

		// Read in a single step
		if (bytes_to_read <= capacity - beg_index_)
		{
			memcpy(data, data_ + beg_index_, bytes_to_read);
			beg_index_ += bytes_to_read;
			if (beg_index_ == capacity) 
				beg_index_ = 0;
		}
		// Read in two steps
		else
		{
			size_t size_1 = capacity - beg_index_;
			memcpy(data, data_ + beg_index_, size_1);
			size_t size_2 = bytes_to_read - size_1;
			memcpy(data + size_1, data_, size_2);
			beg_index_ = size_2;
		}

		size_ -= bytes_to_read;
		return bytes_to_read;
	}

private:
	size_t beg_index_, end_index_, size_, capacity_;
	char *data_;
};
