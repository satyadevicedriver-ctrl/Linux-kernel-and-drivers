# Power Management Hooks Driver (Suspend/Resume)

## 📌 Summary
This driver demonstrates Linux kernel **power management callbacks**:
- `suspend()` – called when the system enters sleep/standby
- `resume()` – called when the system wakes up

It’s attached to a **platform driver** and triggers logs during power events.

---

## 🛠 Skills Demonstrated
- `struct dev_pm_ops` usage
- PM callback binding to `.driver.pm`
- Device Tree + Platform Driver integration
- Basic resume path hardware bring-up structure

---

## 📂 File Structure
