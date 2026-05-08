import { useState } from "react";
import { 
  useListKeys, 
  getListKeysQueryKey, 
  useCreateKey, 
  useRevokeKey, 
  useDeleteKey 
} from "@workspace/api-client-react";
import { useQueryClient } from "@tanstack/react-query";
import { useForm } from "react-hook-form";
import { z } from "zod";
import { zodResolver } from "@hookform/resolvers/zod";
import { format, formatDistanceToNow, isPast } from "date-fns";
import { 
  Copy, 
  Trash2, 
  Ban, 
  Plus, 
  Key as KeyIcon, 
  RefreshCcw, 
  Search,
  Clock,
  AlertCircle
} from "lucide-react";
import { toast } from "sonner";

import { Button } from "@/components/ui/button";
import { Input } from "@/components/ui/input";
import { Form, FormControl, FormField, FormItem, FormLabel, FormMessage } from "@/components/ui/form";
import { Dialog, DialogContent, DialogDescription, DialogFooter, DialogHeader, DialogTitle, DialogTrigger } from "@/components/ui/dialog";
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from "@/components/ui/select";
import { Badge } from "@/components/ui/badge";
import { Skeleton } from "@/components/ui/skeleton";
import { Table, TableBody, TableCell, TableHead, TableHeader, TableRow } from "@/components/ui/table";
import { AlertDialog, AlertDialogAction, AlertDialogCancel, AlertDialogContent, AlertDialogDescription, AlertDialogFooter, AlertDialogHeader, AlertDialogTitle, AlertDialogTrigger } from "@/components/ui/alert-dialog";
import { Tooltip, TooltipContent, TooltipTrigger } from "@/components/ui/tooltip";

const createKeySchema = z.object({
  duration: z.enum(["1h", "1d", "1w", "1m"], {
    required_error: "Please select a duration.",
  }),
  label: z.string().optional(),
});

export default function Dashboard() {
  const queryClient = useQueryClient();
  const [search, setSearch] = useState("");
  const [createDialogOpen, setCreateDialogOpen] = useState(false);

  const { data: keys, isLoading, refetch, isRefetching } = useListKeys({
    query: { queryKey: getListKeysQueryKey() }
  });

  const createMutation = useCreateKey();
  const revokeMutation = useRevokeKey();
  const deleteMutation = useDeleteKey();

  const form = useForm<z.infer<typeof createKeySchema>>({
    resolver: zodResolver(createKeySchema),
    defaultValues: {
      duration: "1w",
      label: "",
    },
  });

  const handleCreate = (data: z.infer<typeof createKeySchema>) => {
    createMutation.mutate({ data: { duration: data.duration, label: data.label || null } }, {
      onSuccess: (newKey) => {
        queryClient.invalidateQueries({ queryKey: getListKeysQueryKey() });
        setCreateDialogOpen(false);
        form.reset();
        toast.success("Key Generated Successfully", {
          description: "The new license key is ready to use.",
        });
        copyToClipboard(newKey.key, "New key copied to clipboard");
      },
      onError: (err) => {
        toast.error("Failed to generate key", {
          description: err.data?.error || "Unknown error occurred"
        });
      }
    });
  };

  const handleRevoke = (id: number) => {
    revokeMutation.mutate({ id }, {
      onSuccess: () => {
        queryClient.invalidateQueries({ queryKey: getListKeysQueryKey() });
        toast.success("Key Revoked", { description: "The license key has been invalidated." });
      },
      onError: (err) => {
        toast.error("Failed to revoke key", { description: err.data?.error || "Unknown error occurred" });
      }
    });
  };

  const handleDelete = (id: number) => {
    deleteMutation.mutate({ id }, {
      onSuccess: () => {
        queryClient.invalidateQueries({ queryKey: getListKeysQueryKey() });
        toast.success("Key Deleted", { description: "The license key has been permanently removed." });
      },
      onError: (err) => {
        toast.error("Failed to delete key", { description: err.data?.error || "Unknown error occurred" });
      }
    });
  };

  const copyToClipboard = (text: string, message = "Copied to clipboard") => {
    navigator.clipboard.writeText(text);
    toast.success(message);
  };

  const filteredKeys = keys?.filter(k => 
    k.key.toLowerCase().includes(search.toLowerCase()) || 
    (k.label && k.label.toLowerCase().includes(search.toLowerCase()))
  ) || [];

  return (
    <div className="space-y-6">
      <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-4">
        <div>
          <h1 className="text-2xl font-bold font-mono tracking-tight">Active Licenses</h1>
          <p className="text-sm text-muted-foreground">Manage and generate API license keys.</p>
        </div>
        
        <div className="flex items-center gap-2">
          <Button 
            variant="outline" 
            size="icon" 
            onClick={() => refetch()} 
            disabled={isRefetching}
            className="border-border/50 bg-card hover:bg-secondary"
          >
            <RefreshCcw className={`h-4 w-4 ${isRefetching ? 'animate-spin text-primary' : ''}`} />
          </Button>

          <Dialog open={createDialogOpen} onOpenChange={setCreateDialogOpen}>
            <DialogTrigger asChild>
              <Button className="font-mono text-xs tracking-wider gap-2">
                <Plus className="h-4 w-4" />
                GENERATE KEY
              </Button>
            </DialogTrigger>
            <DialogContent className="sm:max-w-[425px] bg-card border-border/50">
              <DialogHeader>
                <DialogTitle className="font-mono">Generate New License</DialogTitle>
                <DialogDescription>
                  Create a new authentication key with a specific duration limit.
                </DialogDescription>
              </DialogHeader>
              
              <Form {...form}>
                <form onSubmit={form.handleSubmit(handleCreate)} className="space-y-4 pt-4">
                  <FormField
                    control={form.control}
                    name="duration"
                    render={({ field }) => (
                      <FormItem>
                        <FormLabel className="font-mono text-xs text-muted-foreground">DURATION</FormLabel>
                        <Select onValueChange={field.onChange} defaultValue={field.value}>
                          <FormControl>
                            <SelectTrigger className="font-mono bg-background/50">
                              <SelectValue placeholder="Select duration" />
                            </SelectTrigger>
                          </FormControl>
                          <SelectContent>
                            <SelectItem value="1h">1 Hour</SelectItem>
                            <SelectItem value="1d">1 Day</SelectItem>
                            <SelectItem value="1w">1 Week</SelectItem>
                            <SelectItem value="1m">1 Month</SelectItem>
                          </SelectContent>
                        </Select>
                        <FormMessage />
                      </FormItem>
                    )}
                  />
                  
                  <FormField
                    control={form.control}
                    name="label"
                    render={({ field }) => (
                      <FormItem>
                        <FormLabel className="font-mono text-xs text-muted-foreground">IDENTIFIER (OPTIONAL)</FormLabel>
                        <FormControl>
                          <Input 
                            placeholder="e.g. Customer A - Prod" 
                            {...field} 
                            className="bg-background/50 font-mono text-sm"
                          />
                        </FormControl>
                        <FormMessage />
                      </FormItem>
                    )}
                  />
                  
                  <DialogFooter className="pt-4 border-t border-border/50 mt-6">
                    <Button type="submit" disabled={createMutation.isPending} className="font-mono w-full">
                      {createMutation.isPending ? "GENERATING..." : "CONFIRM GENERATION"}
                    </Button>
                  </DialogFooter>
                </form>
              </Form>
            </DialogContent>
          </Dialog>
        </div>
      </div>

      <div className="bg-card border border-border/50 rounded-lg overflow-hidden shadow-sm">
        <div className="p-4 border-b border-border/50 flex items-center bg-card/50">
          <div className="relative flex-1 max-w-sm">
            <Search className="absolute left-2.5 top-2.5 h-4 w-4 text-muted-foreground" />
            <Input
              placeholder="Search keys or labels..."
              className="pl-9 bg-background/50 border-border/50 h-9 font-mono text-sm"
              value={search}
              onChange={(e) => setSearch(e.target.value)}
            />
          </div>
        </div>

        <div className="overflow-x-auto">
          <Table>
            <TableHeader className="bg-card/80">
              <TableRow className="border-border/50 hover:bg-transparent">
                <TableHead className="font-mono text-xs">KEY</TableHead>
                <TableHead className="font-mono text-xs">LABEL</TableHead>
                <TableHead className="font-mono text-xs">STATUS</TableHead>
                <TableHead className="font-mono text-xs hidden md:table-cell">EXPIRES</TableHead>
                <TableHead className="text-right font-mono text-xs">ACTIONS</TableHead>
              </TableRow>
            </TableHeader>
            <TableBody>
              {isLoading ? (
                Array.from({ length: 5 }).map((_, i) => (
                  <TableRow key={i} className="border-border/50">
                    <TableCell><Skeleton className="h-5 w-48" /></TableCell>
                    <TableCell><Skeleton className="h-5 w-24" /></TableCell>
                    <TableCell><Skeleton className="h-5 w-16" /></TableCell>
                    <TableCell className="hidden md:table-cell"><Skeleton className="h-5 w-32" /></TableCell>
                    <TableCell><Skeleton className="h-8 w-24 ml-auto" /></TableCell>
                  </TableRow>
                ))
              ) : filteredKeys.length === 0 ? (
                <TableRow className="border-border/50 hover:bg-transparent">
                  <TableCell colSpan={5} className="h-32 text-center">
                    <div className="flex flex-col items-center justify-center text-muted-foreground">
                      <KeyIcon className="h-8 w-8 mb-2 opacity-20" />
                      <p className="font-mono text-sm">NO KEYS FOUND</p>
                    </div>
                  </TableCell>
                </TableRow>
              ) : (
                filteredKeys.map((key) => {
                  const expired = isPast(new Date(key.expiresAt));
                  
                  let statusBadge;
                  if (key.revoked) {
                    statusBadge = <Badge variant="destructive" className="bg-destructive/10 text-destructive border-destructive/20 font-mono text-[10px]">REVOKED</Badge>;
                  } else if (expired) {
                    statusBadge = <Badge variant="secondary" className="bg-muted text-muted-foreground border-border/50 font-mono text-[10px]">EXPIRED</Badge>;
                  } else {
                    statusBadge = <Badge className="bg-primary/10 text-primary border-primary/20 font-mono text-[10px]">ACTIVE</Badge>;
                  }

                  return (
                    <TableRow key={key.id} className="border-border/50 hover:bg-secondary/50 group">
                      <TableCell className="font-mono text-sm py-3">
                        <div className="flex items-center">
                          <span className="truncate w-32 md:w-64" title={key.key}>{key.key}</span>
                        </div>
                      </TableCell>
                      <TableCell>
                        {key.label ? (
                          <span className="text-sm font-medium">{key.label}</span>
                        ) : (
                          <span className="text-muted-foreground/50 text-sm italic">-</span>
                        )}
                      </TableCell>
                      <TableCell>{statusBadge}</TableCell>
                      <TableCell className="hidden md:table-cell text-sm text-muted-foreground">
                        <div className="flex items-center">
                          <Clock className="w-3 h-3 mr-1.5 opacity-50" />
                          {expired ? "Expired" : formatDistanceToNow(new Date(key.expiresAt), { addSuffix: true })}
                        </div>
                        <div className="text-[10px] opacity-50 mt-0.5">{format(new Date(key.expiresAt), "MMM d, yyyy HH:mm")}</div>
                      </TableCell>
                      <TableCell className="text-right">
                        <div className="flex items-center justify-end space-x-1 opacity-100 sm:opacity-0 sm:group-hover:opacity-100 transition-opacity">
                          <Tooltip>
                            <TooltipTrigger asChild>
                              <Button variant="ghost" size="icon" className="h-8 w-8 text-muted-foreground hover:text-foreground" onClick={() => copyToClipboard(key.key)}>
                                <Copy className="h-4 w-4" />
                              </Button>
                            </TooltipTrigger>
                            <TooltipContent>Copy Key</TooltipContent>
                          </Tooltip>

                          {!key.revoked && !expired && (
                            <AlertDialog>
                              <AlertDialogTrigger asChild>
                                <Button variant="ghost" size="icon" className="h-8 w-8 text-yellow-500 hover:text-yellow-600 hover:bg-yellow-500/10">
                                  <Ban className="h-4 w-4" />
                                </Button>
                              </AlertDialogTrigger>
                              <AlertDialogContent className="bg-card border-border/50">
                                <AlertDialogHeader>
                                  <AlertDialogTitle>Revoke Key?</AlertDialogTitle>
                                  <AlertDialogDescription>
                                    This will immediately invalidate the license key. Clients using this key will be rejected. This action cannot be undone.
                                  </AlertDialogDescription>
                                </AlertDialogHeader>
                                <AlertDialogFooter>
                                  <AlertDialogCancel className="bg-background">Cancel</AlertDialogCancel>
                                  <AlertDialogAction onClick={() => handleRevoke(key.id)} className="bg-yellow-600 hover:bg-yellow-700 text-white">Revoke</AlertDialogAction>
                                </AlertDialogFooter>
                              </AlertDialogContent>
                            </AlertDialog>
                          )}

                          <AlertDialog>
                            <AlertDialogTrigger asChild>
                              <Button variant="ghost" size="icon" className="h-8 w-8 text-destructive hover:text-destructive hover:bg-destructive/10">
                                <Trash2 className="h-4 w-4" />
                              </Button>
                            </AlertDialogTrigger>
                            <AlertDialogContent className="bg-card border-border/50">
                              <AlertDialogHeader>
                                <AlertDialogTitle className="flex items-center text-destructive">
                                  <AlertCircle className="w-5 h-5 mr-2" />
                                  Delete Key?
                                </AlertDialogTitle>
                                <AlertDialogDescription>
                                  This will permanently remove the key from the database. It cannot be recovered.
                                </AlertDialogDescription>
                              </AlertDialogHeader>
                              <AlertDialogFooter>
                                <AlertDialogCancel className="bg-background">Cancel</AlertDialogCancel>
                                <AlertDialogAction onClick={() => handleDelete(key.id)} className="bg-destructive hover:bg-destructive/90 text-destructive-foreground">Delete</AlertDialogAction>
                              </AlertDialogFooter>
                            </AlertDialogContent>
                          </AlertDialog>
                        </div>
                      </TableCell>
                    </TableRow>
                  );
                })
              )}
            </TableBody>
          </Table>
        </div>
      </div>
    </div>
  );
}
