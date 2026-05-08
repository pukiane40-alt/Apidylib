import { Router, type IRouter } from "express";
import healthRouter from "./health";
import authRouter from "./auth";
import keysRouter from "./keys";

const router: IRouter = Router();

router.use(healthRouter);
router.use("/auth", authRouter);
router.use("/keys", keysRouter);

export default router;
