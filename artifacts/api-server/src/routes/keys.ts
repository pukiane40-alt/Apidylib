import { Router } from "express";
import crypto from "crypto";
import { db, licenseKeysTable } from "@workspace/db";
import { eq } from "drizzle-orm";
import { CreateKeyBody, ValidateKeyBody, DeleteKeyParams, RevokeKeyParams } from "@workspace/api-zod";
import { requireAuth, type AuthRequest } from "../middlewares/auth";

const router = Router();

function computeExpiry(duration: string): Date {
  const now = new Date();
  switch (duration) {
    case "1h":
      return new Date(now.getTime() + 60 * 60 * 1000);
    case "1d":
      return new Date(now.getTime() + 24 * 60 * 60 * 1000);
    case "1w":
      return new Date(now.getTime() + 7 * 24 * 60 * 60 * 1000);
    case "1m":
      return new Date(now.getTime() + 30 * 24 * 60 * 60 * 1000);
    default:
      return new Date(now.getTime() + 24 * 60 * 60 * 1000);
  }
}

function formatTimeRemaining(expiresAt: Date): string {
  const ms = expiresAt.getTime() - Date.now();
  if (ms <= 0) return "0m remaining";

  const hours = Math.floor(ms / (1000 * 60 * 60));
  const minutes = Math.floor((ms % (1000 * 60 * 60)) / (1000 * 60));

  if (hours >= 24) {
    const days = Math.floor(hours / 24);
    const remHours = hours % 24;
    return `${days}d ${remHours}h remaining`;
  }
  if (hours > 0) {
    return `${hours}h ${minutes}m remaining`;
  }
  return `${minutes}m remaining`;
}

function generateKey(): string {
  const part = () => crypto.randomBytes(4).toString("hex").toUpperCase();
  return `XIT-${part()}-${part()}-${part()}`;
}

function formatKey(k: typeof licenseKeysTable.$inferSelect) {
  return {
    id: k.id,
    key: k.key,
    duration: k.duration,
    label: k.label,
    expiresAt: k.expiresAt.toISOString(),
    revoked: k.revoked,
    createdAt: k.createdAt.toISOString(),
    createdBy: k.createdBy,
  };
}

router.get("/", requireAuth, async (req: AuthRequest, res) => {
  try {
    const keys = await db.select().from(licenseKeysTable).orderBy(licenseKeysTable.createdAt);
    res.json(keys.map(formatKey).reverse());
  } catch (err) {
    req.log.error({ err }, "List keys error");
    res.status(500).json({ error: "Internal server error" });
  }
});

router.post("/", requireAuth, async (req: AuthRequest, res) => {
  const parsed = CreateKeyBody.safeParse(req.body);
  if (!parsed.success) {
    res.status(400).json({ error: "Invalid request body" });
    return;
  }

  const { duration, label } = parsed.data;

  try {
    const key = generateKey();
    const expiresAt = computeExpiry(duration);

    const [created] = await db
      .insert(licenseKeysTable)
      .values({
        key,
        duration,
        label: label ?? null,
        expiresAt,
        revoked: false,
        activatedDeviceId: null,
        createdBy: req.userId!,
      })
      .returning();

    res.status(201).json(formatKey(created));
  } catch (err) {
    req.log.error({ err }, "Create key error");
    res.status(500).json({ error: "Internal server error" });
  }
});

router.post("/validate", async (req, res) => {
  const parsed = ValidateKeyBody.safeParse(req.body);
  if (!parsed.success) {
    res.status(400).json({ error: "Invalid request body" });
    return;
  }

  const { key, deviceId } = parsed.data;

  try {
    const [found] = await db
      .select()
      .from(licenseKeysTable)
      .where(eq(licenseKeysTable.key, key))
      .limit(1);

    if (!found) {
      res.json({
        valid: false,
        expiresAt: null,
        timeRemaining: null,
        message: "Key not found",
        deviceLocked: false,
      });
      return;
    }

    if (found.revoked) {
      res.json({
        valid: false,
        expiresAt: found.expiresAt.toISOString(),
        timeRemaining: null,
        message: "Key has been revoked",
        deviceLocked: false,
      });
      return;
    }

    if (found.expiresAt < new Date()) {
      res.json({
        valid: false,
        expiresAt: found.expiresAt.toISOString(),
        timeRemaining: null,
        message: "Key has expired",
        deviceLocked: !!found.activatedDeviceId,
      });
      return;
    }

    /* ── Device binding logic ── */
    if (deviceId) {
      if (!found.activatedDeviceId) {
        /* First activation — bind this key to the device */
        await db
          .update(licenseKeysTable)
          .set({ activatedDeviceId: deviceId })
          .where(eq(licenseKeysTable.id, found.id));

        res.json({
          valid: true,
          expiresAt: found.expiresAt.toISOString(),
          timeRemaining: formatTimeRemaining(found.expiresAt),
          message: "@xit1299 VIP activated",
          deviceLocked: true,
        });
        return;
      }

      if (found.activatedDeviceId !== deviceId) {
        /* Key is already bound to a different device */
        res.json({
          valid: false,
          expiresAt: found.expiresAt.toISOString(),
          timeRemaining: null,
          message: "Key is already activated on another device",
          deviceLocked: true,
        });
        return;
      }
    }

    /* Device matches (or no deviceId sent) */
    res.json({
      valid: true,
      expiresAt: found.expiresAt.toISOString(),
      timeRemaining: formatTimeRemaining(found.expiresAt),
      message: "@xit1299 VIP activated",
      deviceLocked: !!found.activatedDeviceId,
    });
  } catch (err) {
    req.log.error({ err }, "Validate key error");
    res.status(500).json({ error: "Internal server error" });
  }
});

router.delete("/:id", requireAuth, async (req: AuthRequest, res) => {
  const parsed = DeleteKeyParams.safeParse({ id: parseInt(req.params.id) });
  if (!parsed.success) {
    res.status(400).json({ error: "Invalid key id" });
    return;
  }

  try {
    const [deleted] = await db
      .delete(licenseKeysTable)
      .where(eq(licenseKeysTable.id, parsed.data.id))
      .returning();

    if (!deleted) {
      res.status(404).json({ error: "Key not found" });
      return;
    }

    res.json({ message: "Key deleted successfully" });
  } catch (err) {
    req.log.error({ err }, "Delete key error");
    res.status(500).json({ error: "Internal server error" });
  }
});

router.patch("/:id/revoke", requireAuth, async (req: AuthRequest, res) => {
  const parsed = RevokeKeyParams.safeParse({ id: parseInt(req.params.id) });
  if (!parsed.success) {
    res.status(400).json({ error: "Invalid key id" });
    return;
  }

  try {
    const [updated] = await db
      .update(licenseKeysTable)
      .set({ revoked: true })
      .where(eq(licenseKeysTable.id, parsed.data.id))
      .returning();

    if (!updated) {
      res.status(404).json({ error: "Key not found" });
      return;
    }

    res.json(formatKey(updated));
  } catch (err) {
    req.log.error({ err }, "Revoke key error");
    res.status(500).json({ error: "Internal server error" });
  }
});

export default router;
