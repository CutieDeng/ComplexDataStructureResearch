#pragma once 

#include <type_traits> 
#include <memory> 
#include <string> 
#include <stack> 
#include <tuple> 

enum class Tree23Error {
	IllegalNode, 
	ErrLogic, 
} ; 

template <typename K, typename V, typename = void > 
class Tree23; 

template <typename K> 
using Set23 = Tree23<K, void, void>; 

template <typename K> 
class Tree23<K, void, void> {
	private: 
		using this_type = Tree23<K, void, void>; 
		using key_type = K; 
		using value_type = void; 
	public: 
		bool insert(K &&); 
		bool remove(K const &); 
		bool contain(K const &); 
		std::string format() const; 
	private: 
		struct leaf_node_disappear_err; 
		struct node_type {
			friend this_type; 
			private: 
				unsigned char has_sons : 1; 
				unsigned char cnt : 7; 
				alignas (K) char buffer[2][sizeof (K)]; 
				node_type *ptrs[3]; 
			public: 
				bool insert(K &&); 
				bool remove(K const &); 
				bool contain(K const &); 
				static node_type *from_raw(K &&key) {
					node_type *ans = new node_type; 
					ans->cnt = 1; 
					ans->has_sons = 0; 
					new ((K *)ans->buffer[0]) K (std::move(key)); 
					return ans; 
				}; 
			private: 
				node_type *max_node(); 
			private: 
				typedef void (handle_function_type ) (leaf_node_disappear_err &); 
				handle_function_type unsafe2_handle_left; 
				handle_function_type unsafe2_handle_right; 
				handle_function_type unsafe3_handle_left; 
				handle_function_type unsafe3_handle_middle; 
				handle_function_type unsafe3_handle_right; 
				void remove_max( key_type * ); 
		}; 
		struct node_grow_err {
			bool has_sons; 
			K top; 
			K others[2]; 
			node_type *ptrs[4];
		}; 
		struct leaf_node_disappear_err {
			bool has; 
			node_type *ptr; 
		}; 
	private: 
		node_type *root_ {}; 
	public: 
		~Tree23(); 
}; 

template <typename K> 
bool Tree23<K, void, void >::node_type::insert(K &&key) {
	if (cnt == 1) {
		K &k = *(K *)this->buffer[0];
		if (k == key) {
			return false; 
		} 
		bool less = k < key; 
		if (!has_sons) {
			cnt = 2; 
			if (less) {
				new ((K *) this->buffer[1]) K (std::move(key)); 
			} else {
				new ((K *) this->buffer[1]) K (std::move(k)); 
				k.~K(); 
				new (&k) K (std::move(key)); 
			} 
		} else {
			try {
				if (less) {
					return ptrs[1]->insert(std::move(key)); 
				} else {
					return ptrs[0]->insert(std::move(key)); 
				} 
			} catch (node_grow_err &err) {
				int index; 
				if (less) {
					new ((K *) this->buffer[1]) K (std::move(err.top)); 
					index = 1; 
				} else {
					new ((K *) this->buffer[1]) K (std::move(*(K *)this->buffer[0])); 
					((K *)this->buffer[0])->~K(); 
					new ((K *) this->buffer[0]) K (std::move(err.top)); 
					index = 0; 
					ptrs[2] = ptrs[1]; 
				} 
				delete ptrs[index]; 
				ptrs[index] = node_type::from_raw(std::move(err.others[0])); 
				ptrs[index+1] = node_type::from_raw(std::move(err.others[1])); 
				if (err.has_sons) {
					// ptrs[1]->ptrs = { err.ptrs[0], err.ptrs[1], nullptr }; 
					ptrs[index]->ptrs[0] = err.ptrs[0]; 
					ptrs[index]->ptrs[1] = err.ptrs[1]; 
					ptrs[index]->has_sons = 1; 
					ptrs[index+1]->ptrs[0] = err.ptrs[2]; 
					ptrs[index+1]->ptrs[1] = err.ptrs[3]; 
					ptrs[index+1]->has_sons = 1; 
				} 
				cnt = 2; 
			} 
		} 	
		return true; 
	} else if (cnt == 2) {
		K &l = *(K *)this->buffer[0]; 
		K &r = *(K *)this->buffer[1]; 
		int cmp; 
		if (key == l) {
			return false; 
		} else if (key < l) {
			// the left branch 
			cmp = 0; 
		} else if (key == r) {
			return false; 
		} else if (key < r) {
			// the mid branch 
			cmp = 1; 
		} else {
			// the right branch 
			cmp = 2; 
		} 
		if (this->has_sons) {
			// delegate to the right son! 
			auto ptr = this->ptrs[cmp]; 
			try {
				return ptr->insert(std::move(key)); 
			} catch (node_grow_err &err) {
				delete ptrs[cmp]; 
				node_type *l1, *l2; 
				l1 = from_raw(std::move(err.others[0])); 
				l2 = from_raw(std::move(err.others[1])); 
				if (err.has_sons) {
					l1->ptrs[0] = err.ptrs[0]; 
					l1->ptrs[1] = err.ptrs[1]; 
					l2->ptrs[0] = err.ptrs[2]; 
					l2->ptrs[1] = err.ptrs[3]; 
					l1->has_sons = 1; 
					l2->has_sons = 1; 
				} 
				switch (cmp) {
					case 0: {
						auto err2 = (node_grow_err ) {
							.has_sons = true, 
							.top = std::move(l), 
							.others = { std::move(err.top), std::move(r) }, 
						};  
						err2.ptrs[0] = l1; 
						err2.ptrs[1] = l2; 
						err2.ptrs[2] = ptrs[1]; 
						err2.ptrs[3] = ptrs[2]; 
						l.~K(); 
						r.~K(); 
						throw err2; 
					}; 
					case 1: {
						auto err2 = (node_grow_err ) {
							.has_sons = true, 
							.top = std::move(err.top), 
							.others = { std::move(l), std::move(r) }, 
						};  
						err2.ptrs[0] = ptrs[0]; 
						err2.ptrs[1] = l1; 
						err2.ptrs[2] = l2; 
						err2.ptrs[3] = ptrs[2]; 
						l.~K(); 
						r.~K(); 
						throw err2; 
					}
					case 2: {
						auto err2 = (node_grow_err ) {
							.has_sons = true, 
							.top = std::move(r), 
							.others = { std::move(l), std::move(err.top) }, 
						};  
						err2.ptrs[0] = ptrs[0]; 
						err2.ptrs[1] = ptrs[1]; 
						err2.ptrs[2] = l1; 
						err2.ptrs[3] = l2; 
						l.~K(); 
						r.~K(); 
						throw err2; 
					}
					default: {
						throw Tree23Error::IllegalNode; 
				  	}
				} 
			} 
		} else {
			switch (cmp) {
				case 0: {
					auto err = (node_grow_err) {
						.has_sons = false, 
						.top = std::move(l), 
						.others = { std::move(key), std::move(r) }, 
					}; 
					l.~K(); 
					r.~K(); 
					throw err; 
				}
				case 1: {
					auto err = (node_grow_err) {
						.has_sons = false, 
						.top = std::move(key), 
						.others = { std::move(l), std::move(r) }, 
					}; 
					l.~K(); 
					r.~K(); 
					throw err; 
				}
				case 2: {
					auto err = (node_grow_err) {
						.has_sons = false, 
						.top = std::move(r), 
						.others = { std::move(l), std::move(key) }, 
					}; 
					l.~K(); 
					r.~K(); 
					throw err; 
				}
			}
			throw Tree23Error::IllegalNode; 
		} 
	} 
	throw Tree23Error::IllegalNode; 
} 

template <typename T> 
Tree23<T, void, void> &operator += (Tree23<T, void, void> &lhs, T &&rhs) {
	lhs.insert(std::move(rhs)); 
	return lhs; 	
} 

template 
class Tree23<int, void>; 

template <typename K> 
bool Tree23<K, void, void >::insert(K &&key) {
	if (root_) {
		try {
			return root_ -> insert (std::move(key)); 
		} catch (node_grow_err &err) {
			// delete root_; 
			root_->cnt = 1; 
			new ((K *)root_->buffer[0]) K (std::move(err.top)); 
			node_type *l, *r; 
		 	l = node_type::from_raw(std::move(err.others[0])); 
			r = node_type::from_raw(std::move(err.others[1])); 
			root_->ptrs[0] = l; 
			root_->ptrs[1] = r; 
			root_->has_sons = 1; 
			if (err.has_sons) {
				l->ptrs[0] = err.ptrs[0]; 
				l->ptrs[1] = err.ptrs[1]; 
				r->ptrs[0] = err.ptrs[2]; 
				r->ptrs[1] = err.ptrs[3]; 
				l->has_sons = 1; 
				r->has_sons = 1; 
			} 
		} 
	} else {
		root_ = node_type::from_raw(std::move(key)); 
	}	
	return true; 
} 

template <typename K> 
std::string Tree23<K, void, void >::format() const {
	if (!root_) {
		return "null"; 
	} 
	std::string ans; 
	std::stack<std::pair<node_type *, int >> it; 
	it.emplace(root_, 0); 
  while (1) {
		if (it.empty()) break; 
		auto &top = it.top(); 
		++top.second; 
		switch (top.second) {
			case 1: 
			{ 
				if (top.first->has_sons) {
					node_type *s = top.first->ptrs[0]; 
					it.emplace(s, 0); 
				} 
				break; 
			} 
			case 2: 
			{ 
				ans += std::to_string(*(K *)top.first->buffer[0]); 
				ans += ' '; 
				break; 
			} 
			case 3: 
			{
				if (top.first->has_sons) {
					node_type *s = top.first->ptrs[1]; 
					it.emplace(s, 0); 
				} 
				break; 
			}
			case 4: 
			{
				if (top.first->cnt == 2) {
					ans += std::to_string( *(K *) top.first -> buffer[1] ); 
					ans += ' '; 
				} 
				break; 
			} 
			case 5: 
			{
				if (top.first->cnt == 2 && top.first->has_sons) {
					node_type *s = top.first->ptrs[2]; 
					it.emplace(s, 0); 
				} 
				break; 
			} 
			default: 
			{
				it.pop(); 
			} 
		} 
	} 	
	if (!ans.empty()) {
		ans.pop_back(); 
	} 
	return ans; 
} 

template <typename K> 
bool Tree23<K, void, void>::node_type::contain(K const &key) {
	switch (cnt) {
		case 1: 
		case 2: 
		{ 
			auto &k = * (K *) buffer[0]; 
			if (k == key) {
				return true; 
			} else if (!(k < key)) {
				// left branch 
				return has_sons ? ptrs[0]->contain(key) : false; 
			} else {
				// right branch 
				if (cnt == 1) {
					return has_sons ? ptrs[1] -> contain (key) : false; 					
				} else {
					auto &r = * (K *) buffer[1]; 
					if (r == key) {
						return true; 
					} else if (r < key) {
						// right branch 
						return has_sons ? ptrs[2] -> contain (key) : false; 
					} else {
						// mid branch 
						return has_sons ? ptrs[1] -> contain (key) : false; 
					} 
				} 
			} 
		} 
		default: 
			throw Tree23Error::IllegalNode; 
	} 
	throw Tree23Error::ErrLogic; 
} 

template <typename K> 
bool Tree23<K, void, void>::contain(K const &key) {
	return root_ ? root_->contain(key) : false; 
} 

template <typename K> 
bool Tree23<K, void, void>::node_type::remove(K const &key) {
	switch (cnt) {
		case 1: 
		{ 
			K &node = * (K *) buffer[0]; 
			if (!has_sons) {
				if (node == key) {
					// !!! the only one node! 
					// how to kill it? 
					throw leaf_node_disappear_err { .has = false }; 
				} else { 
					return false; 
				} 
			} else {
				if (node == key) {
					// node_type *l = ptrs[0]->max_node(); 
					try {
						ptrs[0]->remove_max((key_type *)buffer[0]); 
					} catch (leaf_node_disappear_err &err) {
						unsafe2_handle_left(err); 
					} 
					return true; 
				} else if (!(node < key)) {
					// left branch 
					try {
						return ptrs[0]->remove(key); 
					} catch (leaf_node_disappear_err &err) {
						unsafe2_handle_left(err); 
						return true; 
					} 
				} else {
					// right branch 
					try {
						return ptrs[1] -> remove (key); 
					} catch (leaf_node_disappear_err &err) {
						unsafe2_handle_right(err); 
						return true; 
					}
				} 
			} 
		} 
		case 2: 
		{
			K &lnode = * (K *) buffer[0]; 
			K &rnode = * (K *) buffer[1]; 
			if (!has_sons) {
				if (lnode == key) {
					lnode.~K(); 
					new (&lnode) K (std::move(rnode)); 	
				} else if (rnode == key) {
					rnode.~K();
				} else {
					return false; 
				}
				cnt = 1; 
				return true; 
			} else {
				int branch; 
				try {
					if (lnode == key) {
						branch = 1; 
						ptrs[0]->remove_max(&lnode); 
						return true; 
					} else if (!(lnode < key)) {
						branch = 1; 
						return ptrs[0]->remove(key); 
					} else if (rnode == key) {
						branch = 2; 
						ptrs[1] -> remove_max(&rnode); 
						return true; 
					} else if (!(rnode < key)) {
						branch = 2; 
						return ptrs[1] -> remove(key); 
					} else {
						branch = 3; 
						return ptrs[2] -> remove(key); 
					} 
				} catch (leaf_node_disappear_err &err) {
					switch (branch) {
						case 1: 
							unsafe3_handle_left(err); 
							break; 
						case 2: 
							unsafe3_handle_middle(err); 
							break; 
						case 3: 
							unsafe3_handle_right(err); 
							break; 
						default: 
							throw Tree23Error::ErrLogic; 
					} 
					return true; 
				} 
			} 
		} 
		default: 
			throw Tree23Error::IllegalNode; 
	} 
	throw Tree23Error::IllegalNode; 
} 

template <typename K> 
bool Tree23<K, void, void>::remove(K const &k) {
	try {
		return root_ ? root_ -> remove(k) : false; 
	} catch (leaf_node_disappear_err &err) {
		delete root_; 
		root_ = err.has ? err.ptr : nullptr; 
		return true; 
	}
} 

template <typename K> 
Tree23<K, void, void>::~Tree23() {
	if (root_) {
		std::deque<node_type *> no_handles {root_}; 
		size_t i; 
		for (i = 0; i < no_handles.size(); ++i) {
			node_type *h = no_handles[i]; 
			((K *)h->buffer[0])->~K(); 
			if (h->has_sons) {
				no_handles.emplace_back(h->ptrs[0]); 
				no_handles.emplace_back(h->ptrs[1]); 
				if (h->cnt == 2) {
					no_handles.emplace_back(h->ptrs[2]); 
					goto here; 
				} 
				goto nohere; 
			} 
			if (h->cnt == 2) {
				here: 
				((K *)h->buffer[1])->~K(); 
			}
			nohere: ;  
		} 
	} 
} 

template <typename K> 
typename Tree23<K, void, void>::node_type *Tree23<K, void, void>::node_type::max_node() {
	if (!has_sons) {
		return this; 
	} 
	if (!(cnt == 1 || cnt == 2)) {
		throw Tree23Error::IllegalNode; 
	}
	return cnt == 1 ? ptrs[1]->max_node() : ptrs[2]->max_node(); 
} 

template <typename K> 
void Tree23<K, void, void>::node_type::unsafe2_handle_left(leaf_node_disappear_err &err) {
	if (ptrs[1] -> cnt == 1) {
		node_type *result = ptrs[0]; 
		result->cnt = 2; 
		result->has_sons = err.has; 
		new ((key_type *) result->buffer[0]) key_type (std::move( * (key_type *) buffer[0] )); 
		((key_type *)buffer[0])->~key_type(); 	
		new ((key_type *) result->buffer[1]) key_type (std::move( * (key_type *) ptrs[1] -> buffer[0])); 
		( (key_type *) ptrs[1] -> buffer[0] ) -> ~key_type(); 
		if (err.has) {
			result->ptrs[0] = err.ptr; 
			result->ptrs[1] = ptrs[1]->ptrs[0]; 
			result->ptrs[2] = ptrs[1]->ptrs[1]; 
		} 
		delete ptrs[1]; 
		throw leaf_node_disappear_err { .has = true, .ptr = result, }; 
	} else if (ptrs[1] -> cnt == 2) {
		ptrs[0] -> has_sons = err.has; 
		ptrs[0] -> cnt = 1; 
		auto &unique = * (key_type *) buffer[0]; 
		new ( (key_type *) ptrs[0] -> buffer[0] ) key_type ( std::move ( unique ) ); 
		unique.~key_type(); 
		ptrs[1]->cnt = 1; 
		new ( &unique ) key_type ( std::move ( * (key_type * ) ptrs[1] -> buffer[0] ) ); 
		((key_type *)ptrs[1]->buffer[0])->~key_type(); 
		new ((key_type *) ptrs[1]->buffer[0] ) key_type(std::move(*(key_type *) ptrs[1]->buffer[1])); 
		((key_type *)ptrs[1]->buffer[1])->~key_type(); 
		if (err.has) {
			ptrs[0] -> ptrs[0] = err.ptr; 
			ptrs[0] -> ptrs[1] = ptrs[1] -> ptrs[0]; 
			ptrs[1] -> ptrs[0] = ptrs[1] -> ptrs[1]; 
			ptrs[1] -> ptrs[1] = ptrs[1] -> ptrs[2]; 
		} 
	} else {
		throw Tree23Error::IllegalNode; 
	} 
} 

template <typename K> 
void Tree23<K, void, void>::node_type::unsafe2_handle_right(leaf_node_disappear_err &err) {
	if (ptrs[0] -> cnt == 1) {
		node_type *result = ptrs[1]; 
		result->cnt = 2; 
		result->has_sons = err.has; 
		new ((key_type *) result->buffer[1]) key_type (std::move( * (key_type *) buffer[0] )); 
		((key_type *)buffer[0])->~key_type(); 	
		new ((key_type *) result->buffer[0]) key_type (std::move( * (key_type *) ptrs[0] -> buffer[0])); 
		( (key_type *) ptrs[0] -> buffer[0] ) -> ~key_type(); 
		if (err.has) {
			result->ptrs[0] = ptrs[0]->ptrs[0]; 
			result->ptrs[1] = ptrs[0]->ptrs[1]; 
			result->ptrs[2] = err.ptr; 
		} 
		delete ptrs[0]; 
		throw leaf_node_disappear_err { .has = true, .ptr = result, }; 
	} else if (ptrs[0] -> cnt == 2) {
		ptrs[1] -> has_sons = err.has; 
		ptrs[1] -> cnt = 1; 
		auto &unique = * (key_type *) buffer[0]; 
		new ( (key_type *) ptrs[1] -> buffer[0] ) key_type ( std::move ( unique ) ); 
		unique.~key_type(); 
		ptrs[0]->cnt = 1; 
		new ( &unique ) key_type ( std::move ( * (key_type * ) ptrs[0] -> buffer[1] ) ); 
		((key_type *)ptrs[0]->buffer[1])->~key_type(); 
		if (err.has) {
			ptrs[1] -> ptrs[0] = ptrs[0] -> ptrs[2]; 
			ptrs[1] -> ptrs[1] = err.ptr; 
		} 
	} else {
		throw Tree23Error::IllegalNode; 
	} 
} 

template <typename K> 
void Tree23<K, void, void>::node_type::unsafe3_handle_left(leaf_node_disappear_err &err) {
	if (ptrs[1] -> cnt == 2) {
		unsafe2_handle_left(err); 
	} else if (ptrs[1] -> cnt == 1) {
		ptrs[0]->cnt = 2; 
		ptrs[0]->has_sons = err.has; 
		new ((key_type *) ptrs[0]->buffer[0]) key_type (std::move(*(key_type *)buffer[0])); 
		( (key_type *) buffer[0] )->~key_type(); 
		new ((key_type *) ptrs[0]->buffer[1]) key_type (std::move(*(key_type *)ptrs[1]->buffer[0])); 
		( (key_type *) ptrs[1] -> buffer[0] ) -> ~key_type(); 
		cnt = 1; 
		new ((key_type *) buffer[0]) key_type (std::move(*(key_type *)buffer[1])); 	
		((key_type *)buffer[1]) -> ~key_type(); 
		if (err.has) {
			ptrs[0]->ptrs[0] = err.ptr; 
			ptrs[0]->ptrs[1] = ptrs[1]->ptrs[0];
			ptrs[0]->ptrs[2] = ptrs[1]->ptrs[1]; 		
		} 
		delete ptrs[1]; 
		ptrs[1] = ptrs[2]; 
	} else {
		throw Tree23Error::IllegalNode; 
	} 
} 

template <typename K> 
void Tree23<K, void, void>::node_type::unsafe3_handle_middle(leaf_node_disappear_err &err) {
	if (ptrs[0]->cnt == 2) {
		// attempt to handle it with left subtree, 
		ptrs[1] -> cnt = 1; 
		ptrs[1] -> has_sons = err.has; 
		new (ptrs[1]->buffer[0]) key_type (std::move(*(key_type *)buffer[0])); 
		( (key_type *) buffer[0] ) -> ~ key_type(); 
		ptrs[0]->cnt = 1; 
		new (buffer[0]) key_type (std::move( * (key_type *) ptrs[0] -> buffer[1] )); 
		((key_type *)ptrs[0]->buffer[1])->~key_type(); 
		if (err.has) {
			ptrs[1] -> ptrs[0] = ptrs[0] -> ptrs[2]; 
			ptrs[1] -> ptrs[1] = err.ptr; 
		} 
	} else if (ptrs[2] -> cnt == 2) {
		// attempt to handle it with right subtree. 
		ptrs[1] -> cnt = 1; 
		ptrs[1] -> has_sons = err.has; 
		new (ptrs[1]->buffer[0]) key_type (std::move(*(key_type *)buffer[0])); 
		( (key_type *) buffer[0] ) -> ~ key_type(); 
		ptrs[2] -> cnt = 1; 
		new (buffer[0]) key_type (std::move( * (key_type *) ptrs[2] -> buffer[0] )); 
		((key_type *)ptrs[2]->buffer[0])->~key_type(); 
		new (ptrs[2]->buffer[0]) key_type (std::move( * (key_type *) ptrs[2] -> buffer[1] )); 
		((key_type *)ptrs[2]->buffer[1])->~key_type(); 
		if (err.has) {
			ptrs[1] -> ptrs[0] = err.ptr; 
			ptrs[1] -> ptrs[1] = ptrs[2] -> ptrs[0]; 
			ptrs[2] -> ptrs[0] = ptrs[2] -> ptrs[1]; 
			ptrs[2] -> ptrs[1] = ptrs[2] -> ptrs[2]; 
		} 
	} else if (ptrs[0]->cnt == 1) {
		// normal situation 
		delete ptrs[1]; 
		new (ptrs[0]->buffer[1]) key_type (std::move(*(key_type *)buffer[0])); 
		((key_type *)buffer[0]) -> ~ key_type(); 
		ptrs[0] -> cnt = 2; 
		if (err.has) {
			ptrs[0]->ptrs[2] = err.ptr;
		} 
		cnt = 1; 
		new (buffer[0]) key_type (std::move(*(key_type *)buffer[1])); 
		((key_type *) buffer[1])->~key_type();  
		ptrs[1] = ptrs[2]; 		
	} else {
		throw Tree23Error::IllegalNode; 
	} 
} 

template <typename K> 
void Tree23<K, void, void>::node_type::unsafe3_handle_right(leaf_node_disappear_err &err) {
	if (ptrs[1] -> cnt == 2) {
		ptrs[2] -> cnt = 1; 
		ptrs[2] -> has_sons = err.has; 
		ptrs[1] -> cnt = 1; 
		new (ptrs[2]->buffer[0]) key_type (std::move(*(key_type *)buffer[0])); 
		((key_type *)buffer[0])->~key_type(); 
		new (buffer[0]) key_type (std::move(*(key_type *)ptrs[1]->buffer[1])); 
		((key_type *)ptrs[1]->buffer[1])->~key_type(); 
		if (err.has) {
			ptrs[2] -> ptrs[0] = ptrs[1] -> ptrs[2]; 
			ptrs[2] -> ptrs[1] = err . ptr; 
		} 
	} else if (ptrs[1] -> cnt == 1) {
		delete ptrs[2]; 
		cnt = 1; 
		ptrs[1] -> cnt = 2; 
		new (ptrs[1]->buffer[1]) key_type (std::move(*(key_type * ) buffer[1])); 
		((key_type * ) buffer[1]) -> ~ key_type(); 
		if (err.has) {
			ptrs[1] -> ptrs[2] = err.ptr; 
		} 
	} else {
		throw Tree23Error::IllegalNode; 
	} 
}	 

template <typename K> 
void Tree23<K, void, void>::node_type::remove_max( key_type *val ) {
	if (cnt != 1 && cnt != 2) {
		throw Tree23Error::IllegalNode; 
	} 
	if (has_sons) {
		try {
			ptrs[cnt]->remove_max(val); 
		} catch (leaf_node_disappear_err &err) {
			if (cnt == 1) {
				unsafe2_handle_right(err); 
			} else if (cnt == 2) {
				unsafe3_handle_right(err); 
			} else {
				throw Tree23Error::IllegalNode; 
			} 
		} 
	} else {
		val -> ~key_type(); 	
		new (val) key_type ( std::move ( * ( key_type * ) buffer [cnt - 1] ) ); 
		((key_type *) buffer[cnt-1]) -> ~key_type(); 
		if (cnt == 1) {
			throw leaf_node_disappear_err { .has = false }; 
		} else {
			cnt = 1; 
		}
	} 
} 
