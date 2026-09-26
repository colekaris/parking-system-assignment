-- Parking System Database Schema

CREATE TABLE ParkingLot (
    lot_id INT PRIMARY KEY,
    name VARCHAR(100),
    total_slots INT,
    available_slots INT,
    rate_per_hour DECIMAL(6,2)
);

CREATE TABLE ParkingSlot (
    slot_id INT PRIMARY KEY,
    lot_id INT REFERENCES ParkingLot(lot_id),
    slot_number INT,
    status ENUM('FREE','OCCUPIED')
);

CREATE TABLE Vehicle (
    vehicle_id INT PRIMARY KEY,
    plate_number VARCHAR(20) UNIQUE,
    vehicle_type VARCHAR(20)
);

CREATE TABLE ParkingSession (
    session_id INT PRIMARY KEY,
    vehicle_id INT REFERENCES Vehicle(vehicle_id),
    slot_id INT REFERENCES ParkingSlot(slot_id),
    entry_time DATETIME,
    exit_time DATETIME NULL,
    amount_paid DECIMAL(8,2) NULL,
    status ENUM('ACTIVE','COMPLETED')
);

-- Notes:
-- * A ParkingSession with status = 'ACTIVE' represents a vehicle currently parked.
-- * On exit: fill exit_time and amount_paid, set status = 'COMPLETED',
--   flip the matching ParkingSlot.status to 'FREE', and increment
--   ParkingLot.available_slots.
-- * On entry: pick a FREE ParkingSlot, flip it to 'OCCUPIED', insert a new
--   ParkingSession row, and decrement ParkingLot.available_slots.
