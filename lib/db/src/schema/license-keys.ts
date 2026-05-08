import { pgTable, text, serial, timestamp, boolean, integer } from "drizzle-orm/pg-core";
import { createInsertSchema } from "drizzle-zod";
import { z } from "zod/v4";
import { usersTable } from "./users";

export const licenseKeysTable = pgTable("license_keys", {
  id: serial("id").primaryKey(),
  key: text("key").notNull().unique(),
  duration: text("duration").notNull(),
  label: text("label"),
  expiresAt: timestamp("expires_at", { withTimezone: true }).notNull(),
  revoked: boolean("revoked").notNull().default(false),
  activatedDeviceId: text("activated_device_id"),
  createdBy: integer("created_by").notNull().references(() => usersTable.id),
  createdAt: timestamp("created_at", { withTimezone: true }).notNull().defaultNow(),
});

export const insertLicenseKeySchema = createInsertSchema(licenseKeysTable).omit({ id: true, createdAt: true });
export type InsertLicenseKey = z.infer<typeof insertLicenseKeySchema>;
export type LicenseKey = typeof licenseKeysTable.$inferSelect;
