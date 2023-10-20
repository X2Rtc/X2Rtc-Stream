#ifndef RTC_SEQ_MANAGER_HPP
#define RTC_SEQ_MANAGER_HPP

#include "common.hpp"
#include <limits> // std::numeric_limits
#include <set>

namespace RTC
{
	// T is the base type (uint16_t, uint32_t, ...).
	// N is the max number of bits used in T.
	template<typename T, uint8_t N = 0>
	class SeqManager
	{
	public:
		static constexpr T MaxValue = (N == 0) ? std::numeric_limits<T>::max() : ((1 << N) - 1);

	public:
		struct SeqLowerThan
		{
			bool operator()(const T lhs, const T rhs) const;
		};

		struct SeqHigherThan
		{
			bool operator()(const T lhs, const T rhs) const;
		};

	private:
		static const SeqLowerThan isSeqLowerThan;
		static const SeqHigherThan isSeqHigherThan;

	public:
		static bool IsSeqLowerThan(const T lhs, const T rhs);
		static bool IsSeqHigherThan(const T lhs, const T rhs);

	public:
		SeqManager() = default;

	public:
		void Sync(T input);
		void Drop(T input);
		bool Input(const T input, T& output);
		T GetMaxInput() const;
		T GetMaxOutput() const;

	private:
		void ClearDropped();

	private:
		// Whether at least a sequence number has been inserted.
		bool started{ false };
		T base{ 0 };
		T maxOutput{ 0 };
		T maxInput{ 0 };
		std::set<T, SeqLowerThan> dropped;
	};
} // namespace RTC

#endif
