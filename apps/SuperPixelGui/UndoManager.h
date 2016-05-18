#ifndef UNDOMANAGER_H
#define UNDOMANAGER_H

#include <array>

// T -> What type
// N -> Length of history
template <typename T, int N>
class UndoManager {
public:
	constexpr UndoManager() noexcept: beg(0),end(0),pos(-1){}
	UndoManager(const UndoManager<T,N> & other): beg(other.beg), end(other.end), pos(other.pos){
		for (int i = 0; i < N; i++) {
			array[i] = other.array[i];
		}
	}
	void pushItem(){
		pos = (pos+1)%N;
		end = (pos+1)%N; //Always: pos < end !
		if (beg == end) beg = (beg+1)%N;
	}
	const T* getItem() const{
		if (validIdx(pos))
			return &array[pos];
		return nullptr;
	}
	T* getItem(){
		if (validIdx(pos))
			return &array[pos];
		return nullptr;
	}
	void undo(){
		if (undoAvailable()) pos=(pos-1)%N;
	}
	void redo(){
		if (redoAvailable()) pos=(pos+1)%N;
	}
	bool undoAvailable(){
		return validIdx((pos-1)%N);
	}
	bool redoAvailable(){
		return validIdx((pos+1)%N);
	}
protected:
	bool validIdx(int idx) const{
		if (idx >= N || idx <0) return false;
		if (beg < end){
			return beg<= idx && idx < end;
		}else if (end < beg){
			return beg <= idx || idx < end;
		}
		return false;
	}
private:
	std::array<T,N> array;
	int beg, end, pos; //beg <= pos <end
};
#endif
