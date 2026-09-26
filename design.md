# Algorithm

## 1. Check Availability
- Read `available_slots` counter — O(1).
- Return "Available" if `available_slots > 0`, else "Full".

## 2. Vehicle Entry
1. Check availability. If full, reject entry.
2. Pop the lowest free slot number from a min-heap of free slots.
3. Record `{plate_number, slot_number, entry_time}` in a lookup table keyed by plate.
4. Decrement `available_slots`.
5. Return the assigned slot number / ticket.

## 3. Vehicle Exit
1. Look up the vehicle's record by plate number — O(1) via hash map.
2. Compute `duration = exit_time - entry_time`.
3. Compute `amount = rate_per_hour * ceil(duration_in_hours)`.
4. Remove the record from the lookup table.
5. Push the freed slot number back onto the min-heap.
6. Increment `available_slots`.
7. Return the amount due.

# Data Structures

| Structure | Purpose | Why |
|---|---|---|
| `unordered_map<string, VehicleRecord>` | plate -> {slot, entry_time} | O(1) lookup/insert/delete on exit |
| min-heap (`priority_queue`) of free slot numbers | tracks which slots are open | O(log n) to grab the lowest free slot, keeps allocation compact |
| `int available_slots` / `int total_slots` | fast availability check | O(1), avoids scanning all slots |
| `struct VehicleRecord { string plate; int slot; time_t entry_time; }` | one parked vehicle's state | groups everything needed for exit/billing |

This gives O(1) availability checks, O(1) entry/exit lookup, and O(log n) slot assignment.
